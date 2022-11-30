#include "ws2812b.h"
#include "../leds.h"
#include "../interfaces/i2s.h"

#include <logging.h>

#if CONFIG_LEDS_I2S_ENABLED
  /*
   * Use 1.25us WS2812B bits divided into four periods in 1000/1110 form for 0/1 bits.
   *
   * https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
   */
   #define WS2812B_LUT(x) (\
       (((x >> 0) & 0x1) ? 0b0000000000001110 : 0b0000000000001000) \
     | (((x >> 1) & 0x1) ? 0b0000000011100000 : 0b0000000010000000) \
     | (((x >> 2) & 0x1) ? 0b0000111000000000 : 0b0000100000000000) \
     | (((x >> 3) & 0x1) ? 0b1110000000000000 : 0b1000000000000000) \
   )

  static const uint16_t ws2812b_lut[] = {
    [0b0000] = WS2812B_LUT(0b0000),
    [0b0001] = WS2812B_LUT(0b0001),
    [0b0010] = WS2812B_LUT(0b0010),
    [0b0011] = WS2812B_LUT(0b0011),
    [0b0100] = WS2812B_LUT(0b0100),
    [0b0101] = WS2812B_LUT(0b0101),
    [0b0110] = WS2812B_LUT(0b0110),
    [0b0111] = WS2812B_LUT(0b0111),
    [0b1000] = WS2812B_LUT(0b1000),
    [0b1001] = WS2812B_LUT(0b1001),
    [0b1010] = WS2812B_LUT(0b1010),
    [0b1011] = WS2812B_LUT(0b1011),
    [0b1100] = WS2812B_LUT(0b1100),
    [0b1101] = WS2812B_LUT(0b1101),
    [0b1110] = WS2812B_LUT(0b1110),
    [0b1111] = WS2812B_LUT(0b1111),
  };

  void leds_protocol_ws2812b_i2s_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union ws2812b_pixel pixel = ws2812b_pixel(pixels[index], index, limit);

    // 16-bit little-endian
    buf[0] = ws2812b_lut[(pixel._grb >> 20) & 0xf];
    buf[1] = ws2812b_lut[(pixel._grb >> 16) & 0xf];
    buf[2] = ws2812b_lut[(pixel._grb >> 12) & 0xf];
    buf[3] = ws2812b_lut[(pixel._grb >>  8) & 0xf];
    buf[4] = ws2812b_lut[(pixel._grb >>  4) & 0xf];
    buf[5] = ws2812b_lut[(pixel._grb >>  0) & 0xf];
  }
#endif
