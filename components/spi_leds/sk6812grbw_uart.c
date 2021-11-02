#include "sk6812grbw.h"
#include "spi_leds.h"

#include <logging.h>

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
     0b100110 \
   | (((x >> 0) & 0x1) ? 0 : 0b010000) \
   | (((x >> 1) & 0x1) ? 0 : 0b000001) \
 )

static const uint8_t sk6812_lut[] = {
  [0b00] = SK6812_LUT(0b00),
  [0b01] = SK6812_LUT(0b01),
  [0b10] = SK6812_LUT(0b10),
  [0b11] = SK6812_LUT(0b11),
};

static const struct uart1_options uart1_options = {
  .clock_div    = UART1_BAUD_3333333,
  .data_bits    = UART1_DATA_BITS_6,
  .parity_bits  = UART1_PARTIY_DISABLE,
  .stop_bits    = UART1_STOP_BITS_1,
  .inverted     = true,
};

int spi_leds_tx_uart_sk6812grbw(const struct spi_leds_options *options, union sk6812grbw_pixel *pixels, unsigned count)
{
  uint8_t buf[16];
  int err;

  if ((err = uart1_open(options->uart1, uart1_options))) {
    LOG_ERROR("uart1_open");
    return err;
  }

  if (options->gpio_out) {
    gpio_out_set(options->gpio_out, options->gpio_out_pins);
  }

  for (unsigned i = 0; i < count; i++) {
    uint32_t grbw = pixels[i].grbw;

    buf[0]  = sk6812_lut[(grbw >> 30) & 0x3];
    buf[1]  = sk6812_lut[(grbw >> 28) & 0x3];
    buf[2]  = sk6812_lut[(grbw >> 26) & 0x3];
    buf[3]  = sk6812_lut[(grbw >> 24) & 0x3];
    buf[4]  = sk6812_lut[(grbw >> 22) & 0x3];
    buf[5]  = sk6812_lut[(grbw >> 20) & 0x3];
    buf[6]  = sk6812_lut[(grbw >> 18) & 0x3];
    buf[7]  = sk6812_lut[(grbw >> 16) & 0x3];
    buf[8]  = sk6812_lut[(grbw >> 14) & 0x3];
    buf[9]  = sk6812_lut[(grbw >> 12) & 0x3];
    buf[10] = sk6812_lut[(grbw >> 10) & 0x3];
    buf[11] = sk6812_lut[(grbw >>  8) & 0x3];
    buf[12] = sk6812_lut[(grbw >>  6) & 0x3];
    buf[13] = sk6812_lut[(grbw >>  4) & 0x3];
    buf[14] = sk6812_lut[(grbw >>  2) & 0x3];
    buf[15] = sk6812_lut[(grbw >>  0) & 0x3];

    if ((err = uart1_write_all(options->uart1, buf, sizeof(buf)))) {
      LOG_ERROR("uart1_write_all");
      goto error;
    }
  }

  if ((err = uart1_mark(options->uart1, SK6812_RESET_US))) {
    LOG_ERROR("uart1_mark");
    goto error;
  }

error:
  if (options->gpio_out) {
    gpio_out_clear(options->gpio_out);
  }

  if ((err = uart1_close(options->uart1))) {
    LOG_ERROR("uart1_close");
    return err;
  }

  return err;
}
