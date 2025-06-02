#include "ws2812b.h"
#include "../leds.h"
#include "../pixel.h"
#include "../interfaces/uart.h"

#include <logging.h>

#if CONFIG_LEDS_UART_ENABLED
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
       0b0001001000010010 \
     | (((x >> 0) & 0x1) ? 0 : 0b0100000000000000) \
     | (((x >> 1) & 0x1) ? 0 : 0b0000100000000000) \
     | (((x >> 2) & 0x1) ? 0 : 0b0000000100000000) \
     | (((x >> 3) & 0x1) ? 0 : 0b0000000001000000) \
     | (((x >> 4) & 0x1) ? 0 : 0b0000000000001000) \
     | (((x >> 5) & 0x1) ? 0 : 0b0000000000000001) \
   )

  static const uint16_t ws2812b_lut[] = {
    [0b000000] = WS2812B_LUT(0b000000),
    [0b000001] = WS2812B_LUT(0b000001),
    [0b000010] = WS2812B_LUT(0b000010),
    [0b000011] = WS2812B_LUT(0b000011),
    [0b000100] = WS2812B_LUT(0b000100),
    [0b000101] = WS2812B_LUT(0b000101),
    [0b000110] = WS2812B_LUT(0b000110),
    [0b000111] = WS2812B_LUT(0b000111),
    [0b001000] = WS2812B_LUT(0b001000),
    [0b001001] = WS2812B_LUT(0b001001),
    [0b001010] = WS2812B_LUT(0b001010),
    [0b001011] = WS2812B_LUT(0b001011),
    [0b001100] = WS2812B_LUT(0b001100),
    [0b001101] = WS2812B_LUT(0b001101),
    [0b001110] = WS2812B_LUT(0b001110),
    [0b001111] = WS2812B_LUT(0b001111),
    [0b010000] = WS2812B_LUT(0b010000),
    [0b010001] = WS2812B_LUT(0b010001),
    [0b010010] = WS2812B_LUT(0b010010),
    [0b010011] = WS2812B_LUT(0b010011),
    [0b010100] = WS2812B_LUT(0b010100),
    [0b010101] = WS2812B_LUT(0b010101),
    [0b010110] = WS2812B_LUT(0b010110),
    [0b010111] = WS2812B_LUT(0b010111),
    [0b011000] = WS2812B_LUT(0b011000),
    [0b011001] = WS2812B_LUT(0b011001),
    [0b011010] = WS2812B_LUT(0b011010),
    [0b011011] = WS2812B_LUT(0b011011),
    [0b011100] = WS2812B_LUT(0b011100),
    [0b011101] = WS2812B_LUT(0b011101),
    [0b011110] = WS2812B_LUT(0b011110),
    [0b011111] = WS2812B_LUT(0b011111),
    [0b100000] = WS2812B_LUT(0b100000),
    [0b100001] = WS2812B_LUT(0b100001),
    [0b100010] = WS2812B_LUT(0b100010),
    [0b100011] = WS2812B_LUT(0b100011),
    [0b100100] = WS2812B_LUT(0b100100),
    [0b100101] = WS2812B_LUT(0b100101),
    [0b100110] = WS2812B_LUT(0b100110),
    [0b100111] = WS2812B_LUT(0b100111),
    [0b101000] = WS2812B_LUT(0b101000),
    [0b101001] = WS2812B_LUT(0b101001),
    [0b101010] = WS2812B_LUT(0b101010),
    [0b101011] = WS2812B_LUT(0b101011),
    [0b101100] = WS2812B_LUT(0b101100),
    [0b101101] = WS2812B_LUT(0b101101),
    [0b101110] = WS2812B_LUT(0b101110),
    [0b101111] = WS2812B_LUT(0b101111),
    [0b110000] = WS2812B_LUT(0b110000),
    [0b110001] = WS2812B_LUT(0b110001),
    [0b110010] = WS2812B_LUT(0b110010),
    [0b110011] = WS2812B_LUT(0b110011),
    [0b110100] = WS2812B_LUT(0b110100),
    [0b110101] = WS2812B_LUT(0b110101),
    [0b110110] = WS2812B_LUT(0b110110),
    [0b110111] = WS2812B_LUT(0b110111),
    [0b111000] = WS2812B_LUT(0b111000),
    [0b111001] = WS2812B_LUT(0b111001),
    [0b111010] = WS2812B_LUT(0b111010),
    [0b111011] = WS2812B_LUT(0b111011),
    [0b111100] = WS2812B_LUT(0b111100),
    [0b111101] = WS2812B_LUT(0b111101),
    [0b111110] = WS2812B_LUT(0b111110),
    [0b111111] = WS2812B_LUT(0b111111),
  };

  void leds_protocol_ws2812b_uart_out(uint16_t buf[4], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union leds_pixel_grb pixel = leds_pixel_grb(pixels[index], index, limit);

    // 16-bit little-endian
    buf[0] = ws2812b_lut[(pixel._grb >> 18) & 0x3f];
    buf[1] = ws2812b_lut[(pixel._grb >> 12) & 0x3f];
    buf[2] = ws2812b_lut[(pixel._grb >>  6) & 0x3f];
    buf[3] = ws2812b_lut[(pixel._grb >>  0) & 0x3f];
  }

#endif
