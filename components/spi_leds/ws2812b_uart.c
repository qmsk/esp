#include "ws2812b.h"
#include "spi_leds.h"

#include <logging.h>

/*
 * Using 7-bit TX-inverted UART at 2.5M baud (0.4us per bit) to generate a WS2812B signal,
 * at three bits per (7-bit) byte: http://mikrokontrolery.blogspot.com/2011/03/Diody-WS2812B-sterowanie-XMega-cz-2.html
 *
 * The ESP8266 UART uses least-significant-bit first bit order, ignoring the most-significant-bit per byte in 7-bit mode.
 *
 * UART idle state is MARK -> high, start bit BREAK -> low, stop bit MARK -> high.
 * UART tx needs to be inverted to match the WS2812B protocol.
 *
 * One start + 7 data bits + stop frame matches 9 WS2812B periods and encodes 3 WS2812B data bits.
 * The start bit and data bits 1, 4 are always 0 -> break -> TX' high.
 * The data bits 0, 3 and 6 encode the WS2812B data bits.
 * The data bits 2, 4 and end bit are always 1 -> mark -> TX' low.
 * The idle period is mark -> TX' low and encodes the WS2812B reset period.
 *
 *           | IDLE  | START | BIT 0 | BIT 1 | BIT 2 | BIT 3 | BIT 4 | BIT 5 | BIT 6 | END   |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 * WS2812B   |Treset |  T0H  |  T0L  |  T0L  |  T1H  |  T1H  |  T1L  |  TXH  |  TX?  |  TXL  |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  TX'      |   L   |   H   |   L   |   L   |   H   |   H   |   L   |   H   |   ?   |   L   |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  UART     |   M   |   B   |   M   |   M   |   B   |   B   |   M   |   B   |   ?   |   M   |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  LUT      |       |       |1 << 0 |1 << 1 |1 << 2 |1 << 3 |1 << 4 |1 << 5 |1 << 6 |       |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
  *  fixed   |       |       |       |   1   |   0   |       |   1   |   0   |       |       |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *  data     |       |       |   0   |       |       |   1   |       |       |   X   |       |
 * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 */

 // using least-significant-bit first bit order
 #define WS2812B_LUT(x) (\
     0b0010010 \
   | (((x >> 0) & 0x1) ? 0 : 0b1000000) \
   | (((x >> 1) & 0x1) ? 0 : 0b0001000) \
   | (((x >> 2) & 0x1) ? 0 : 0b0000001) \
 )

static const uint8_t ws2812b_lut[] = {
  [0b000] = WS2812B_LUT(0b000),
  [0b001] = WS2812B_LUT(0b001),
  [0b010] = WS2812B_LUT(0b010),
  [0b011] = WS2812B_LUT(0b011),
  [0b100] = WS2812B_LUT(0b100),
  [0b101] = WS2812B_LUT(0b101),
  [0b110] = WS2812B_LUT(0b110),
  [0b111] = WS2812B_LUT(0b111),
};

static const struct uart1_options uart1_options = {
  .clock_div    = UART1_BAUD_2500000,
  .data_bits    = UART1_DATA_BITS_7,
  .parity_bits  = UART1_PARTIY_DISABLE,
  .stop_bits    = UART1_STOP_BITS_1,
  .inverted     = true,
};

int spi_leds_tx_uart_ws2812b(const struct spi_leds_options *options, union ws2812b_pixel *pixels, unsigned count)
{
  uint8_t buf[8];
  int err;

  if ((err = uart1_open(options->uart1, uart1_options))) {
    LOG_ERROR("uart1_open");
    return err;
  }

  if (options->gpio_out) {
    gpio_out_set(options->gpio_out, options->gpio_out_pins);
  }

  LOG_INFO("count=%u", count);

  for (unsigned i = 0; i < count; i++) {
    uint32_t grb = pixels[i]._grb;

    buf[0] = ws2812b_lut[(grb >> 21) & 0x7];
    buf[1] = ws2812b_lut[(grb >> 18) & 0x7];
    buf[2] = ws2812b_lut[(grb >> 15) & 0x7];
    buf[3] = ws2812b_lut[(grb >> 12) & 0x7];
    buf[4] = ws2812b_lut[(grb >>  9) & 0x7];
    buf[5] = ws2812b_lut[(grb >>  6) & 0x7];
    buf[6] = ws2812b_lut[(grb >>  3) & 0x7];
    buf[7] = ws2812b_lut[(grb >>  0) & 0x7];

    if ((err = uart1_write_all(options->uart1, buf, sizeof(buf)))) {
      LOG_ERROR("uart1_write_all");
      goto error;
    }
  }

  if ((err = uart1_flush(options->uart1))) {
    LOG_ERROR("uart1_flush");
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
