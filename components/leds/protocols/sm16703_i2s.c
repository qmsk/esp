#include "sm16703.h"
#include "../leds.h"
#include "../interfaces/i2s.h"

#include <logging.h>

#if CONFIG_LEDS_I2S_ENABLED
  /*
   * Use 1.2us SM16703 bits divided into four periods in 1000/1110 form for 0/1 bits.
   *
   * https://www.gree-leds.com/web/userfiles/download/SM16703ICdatasheet.pdf
   */
   #define SM16703_LUT(x) (\
       (((x >> 0) & 0x1) ? 0b0000000000001110 : 0b0000000000001000) \
     | (((x >> 1) & 0x1) ? 0b0000000011100000 : 0b0000000010000000) \
     | (((x >> 2) & 0x1) ? 0b0000111000000000 : 0b0000100000000000) \
     | (((x >> 3) & 0x1) ? 0b1110000000000000 : 0b1000000000000000) \
   )

  static const uint16_t sm16703_lut[] = {
    [0b0000] = SM16703_LUT(0b0000),
    [0b0001] = SM16703_LUT(0b0001),
    [0b0010] = SM16703_LUT(0b0010),
    [0b0011] = SM16703_LUT(0b0011),
    [0b0100] = SM16703_LUT(0b0100),
    [0b0101] = SM16703_LUT(0b0101),
    [0b0110] = SM16703_LUT(0b0110),
    [0b0111] = SM16703_LUT(0b0111),
    [0b1000] = SM16703_LUT(0b1000),
    [0b1001] = SM16703_LUT(0b1001),
    [0b1010] = SM16703_LUT(0b1010),
    [0b1011] = SM16703_LUT(0b1011),
    [0b1100] = SM16703_LUT(0b1100),
    [0b1101] = SM16703_LUT(0b1101),
    [0b1110] = SM16703_LUT(0b1110),
    [0b1111] = SM16703_LUT(0b1111),
  };

  void leds_protocol_sm16703_i2s_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union sm16703_pixel pixel = sm16703_pixel(pixels[index], index, limit);

    // 16-bit little-endian
    buf[0] = sm16703_lut[(pixel._rgb >> 20) & 0xf];
    buf[1] = sm16703_lut[(pixel._rgb >> 16) & 0xf];
    buf[2] = sm16703_lut[(pixel._rgb >> 12) & 0xf];
    buf[3] = sm16703_lut[(pixel._rgb >>  8) & 0xf];
    buf[4] = sm16703_lut[(pixel._rgb >>  4) & 0xf];
    buf[5] = sm16703_lut[(pixel._rgb >>  0) & 0xf];
  }

#endif
