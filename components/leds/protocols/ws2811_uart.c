#include "ws2811.h"
#include "../leds.h"
#include "../pixel.h"
#include "../interfaces/uart.h"

#include <logging.h>

#if CONFIG_LEDS_UART_ENABLED
  /*
   * Using 8-bit TX-inverted UART at 4M baud (0.25us per bit) to generate a WS2811 signal,
   * at two bits per (8-bit) byte.

   * The ESP8266 UART uses least-significant-bit first bit order.
   *
   * UART idle state is MARK -> high, start bit BREAK -> low, stop bit MARK -> high.
   * UART tx needs to be inverted to match the WS2811 protocol.
   *
   * One start + 8 data bits + stop frame matches 10 WS2811 periods and encodes 2 WS2811 data bits.
   * The idle period is mark -> TX' low and encodes the WS2812B reset period.
   *
   *           | IDLE  | START | BIT 0 | BIT 1 | BIT 2 | BIT 3 | BIT 4 | BIT 5 | BIT 6 | BIT 7 | END   |
   * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
   * WS2811    |Treset |  T0H  |  T0L  |  T0L  |  T0L  |  T0L  |  T1H  |  T1H  |  T1H  |  T1H  |  T1L  |
   * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
   *  TX'      |   L   |   H   |   L   |   L   |   L   |   L   |   H   |   H   |   H   |   H   |   L   |
   * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
   *  UART     |   M   |   B   |   M   |   M   |   M   |   M   |   B   |   B   |   B   |   B   |   M   |
   * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
   *  LUT      |       |       |1 << 0 |1 << 1 |1 << 2 |1 << 3 |1 << 4 |1 << 5 |1 << 6 |1 << 7 |       |
   * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
    *  fixed   |       |       |       |       |       |   1   |   0   |       |       |       |       |
   * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
   *  data     |       |       |   1   |   1   |   1   |       |       |  0    |   0   |   0   |       |
   * ----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
   */

   // using least-significant-bit first bit order
   #define WS2811_LUT(x) (\
       0b0000100000001000 \
     | (((x >> 0) & 0x1) ? 0 : 0b1110000000000000) \
     | (((x >> 1) & 0x1) ? 0 : 0b0000011100000000) \
     | (((x >> 2) & 0x1) ? 0 : 0b0000000011100000) \
     | (((x >> 3) & 0x1) ? 0 : 0b0000000000000111) \
   )

  static const uint16_t ws2811_lut[] = {
    [0b0000] = WS2811_LUT(0b0000),
    [0b0001] = WS2811_LUT(0b0001),
    [0b0010] = WS2811_LUT(0b0010),
    [0b0011] = WS2811_LUT(0b0011),
    [0b0100] = WS2811_LUT(0b0100),
    [0b0101] = WS2811_LUT(0b0101),
    [0b0110] = WS2811_LUT(0b0110),
    [0b0111] = WS2811_LUT(0b0111),
    [0b1000] = WS2811_LUT(0b1000),
    [0b1001] = WS2811_LUT(0b1001),
    [0b1010] = WS2811_LUT(0b1010),
    [0b1011] = WS2811_LUT(0b1011),
    [0b1100] = WS2811_LUT(0b1100),
    [0b1101] = WS2811_LUT(0b1101),
    [0b1110] = WS2811_LUT(0b1110),
    [0b1111] = WS2811_LUT(0b1111),
  };

  void leds_protocol_ws2811_uart_rgb_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union leds_pixel_rgb pixel = leds_pixel_rgb(pixels[index], index, limit);

    // 16-bit little-endian
    buf[0]  = ws2811_lut[(pixel._rgb >> 20) & 0xf];
    buf[1]  = ws2811_lut[(pixel._rgb >> 16) & 0xf];
    buf[2]  = ws2811_lut[(pixel._rgb >> 12) & 0xf];
    buf[3]  = ws2811_lut[(pixel._rgb >>  8) & 0xf];
    buf[4]  = ws2811_lut[(pixel._rgb >>  4) & 0xf];
    buf[5]  = ws2811_lut[(pixel._rgb >>  0) & 0xf];
  }

#endif
