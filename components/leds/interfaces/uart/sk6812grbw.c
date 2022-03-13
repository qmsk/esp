#include "../sk6812grbw.h"
#include "../../leds.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SK6812_TX_TASK_PRIORITY 14

/*
 * Using 6-bit TX-inverted UART at 3.33333333M baud (0.3us per bit) to generate a SK6812 signal,
 * at two bits per (8-bit) byte.
 *
 * The ESP8266 UART uses least-significant-bit first bit order, ignoring the most-significant-bits per byte in 6-bit mode.
 *
 * UART idle state is MARK -> high, start bit BREAK -> low, stop bit MARK -> high.
 * UART tx needs to be inverted to match the WS2812B protocol.
 *
 * One start + 6 data bits + stop frame matches 8 SK6812 periods and encodes 2 SK6812 data bits.
 * The start bit and data bits 4 are always 0 -> break -> TX' high.
 * The data bits 0, 3 and 6 encode the WS2812B data bits.
 * The data bits 2, 4 and stop bit are always 1 -> mark -> TX' low.
 * The idle period is mark -> TX' low and encodes the WS2812B reset period.
 *
 *           | IDLE  | START | BIT 0 | BIT 1 | BIT 2 | BIT 3 | BIT 4 | BIT 5 |  END  |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 * SK6812    |Treset |  T0H  |  T0L  |  T0L  |  T0L  |  T1H  |  T1H  |  T1L  |  T1L  |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  TX'      |   L   |   H   |   L   |   L   |   L   |   H   |   H   |   L   |   L   |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  UART     |   M   |   B   |   M   |   M   |   M   |   B   |   B   |   M   |   M   |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  LUT      |       |       |1 << 0 |1 << 1 |1 << 2 |1 << 3 |1 << 4 |1 << 5 |       |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  fixed    |       |       |       |   1   |   1   |   0   |       |   1   |       |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  bit      |       |       |   1   |       |       |       |   0   |       |       |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  data     |       |       |   0   |       |       |       |   1   |       |       |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 */
 #define SK6812_RESET_US 80

 // uart uses least-significant-bit first bit order
 #define SK6812_LUT(x) (\
     0b0010011000100110 \
   | (((x >> 0) & 0x1) ? 0 : 0b0001000000000000) \
   | (((x >> 1) & 0x1) ? 0 : 0b0000000100000000) \
   | (((x >> 2) & 0x1) ? 0 : 0b0000000000010000) \
   | (((x >> 3) & 0x1) ? 0 : 0b0000000000000001) \
 )

static const uint16_t sk6812_lut[] = {
  [0b0000] = SK6812_LUT(0b0000),
  [0b0001] = SK6812_LUT(0b0001),
  [0b0010] = SK6812_LUT(0b0010),
  [0b0011] = SK6812_LUT(0b0011),
  [0b0100] = SK6812_LUT(0b0100),
  [0b0101] = SK6812_LUT(0b0101),
  [0b0110] = SK6812_LUT(0b0110),
  [0b0111] = SK6812_LUT(0b0111),
  [0b1000] = SK6812_LUT(0b1000),
  [0b1001] = SK6812_LUT(0b1001),
  [0b1010] = SK6812_LUT(0b1010),
  [0b1011] = SK6812_LUT(0b1011),
  [0b1100] = SK6812_LUT(0b1100),
  [0b1101] = SK6812_LUT(0b1101),
  [0b1110] = SK6812_LUT(0b1110),
  [0b1111] = SK6812_LUT(0b1111),
};

int leds_tx_uart_sk6812grbw(const struct leds_interface_uart_options *options, union sk6812grbw_pixel *pixels, unsigned count)
{
  struct uart_options uart_options = {
    .baud_rate    = UART_BAUD_3333333,
    .data_bits    = UART_DATA_6_BITS,
    .parity_bits  = UART_PARITY_DISABLE,
    .stop_bits    = UART_STOP_BITS_1,

    .tx_inverted  = true,

    .pin_mutex    = options->pin_mutex,
  };
  UBaseType_t task_priority = uxTaskPriorityGet(NULL);
  uint16_t buf[8];
  int err;

  if ((err = uart_open(options->uart, uart_options))) {
    LOG_ERROR("uart_open");
    return err;
  }

#if CONFIG_LEDS_GPIO_ENABLED
  if (options->gpio.gpio_out) {
    gpio_out_set(options->gpio.gpio_out, options->gpio.gpio_out_pins);
  }
#endif

  // temporarily raise task priority to ensure uart TX buffer does not starve
  vTaskPrioritySet(NULL, SK6812_TX_TASK_PRIORITY);

  for (unsigned i = 0; i < count; i++) {
    uint32_t grbw = pixels[i].grbw;

    buf[0]  = sk6812_lut[(grbw >> 28) & 0xf];
    buf[1]  = sk6812_lut[(grbw >> 24) & 0xf];
    buf[2]  = sk6812_lut[(grbw >> 20) & 0xf];
    buf[3]  = sk6812_lut[(grbw >> 16) & 0xf];
    buf[4]  = sk6812_lut[(grbw >> 12) & 0xf];
    buf[5]  = sk6812_lut[(grbw >>  8) & 0xf];
    buf[6]  = sk6812_lut[(grbw >>  4) & 0xf];
    buf[7]  = sk6812_lut[(grbw >>  0) & 0xf];

    if ((err = uart_write_all(options->uart, buf, sizeof(buf)))) {
      LOG_ERROR("uart_write_all");
      goto error;
    }
  }

  // restore previous task priority
  vTaskPrioritySet(NULL, task_priority);

  if ((err = uart_mark(options->uart, SK6812_RESET_US))) {
    LOG_ERROR("uart_mark");
    goto error;
  }

error:
#if CONFIG_LEDS_GPIO_ENABLED
  if (options->gpio.gpio_out) {
    gpio_out_clear(options->gpio.gpio_out);
  }
#endif

  if ((err = uart_close(options->uart))) {
    LOG_ERROR("uart_close");
    return err;
  }

  return err;
}
