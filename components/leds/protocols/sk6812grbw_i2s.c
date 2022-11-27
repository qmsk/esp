#include "sk6812grbw.h"
#include "../leds.h"
#include "../interfaces/i2s.h"

#include <logging.h>

#include <stdlib.h>

#if CONFIG_LEDS_I2S_ENABLED
  /*
   * Use 1.25us SK6812 bits divided into four periods in 1000/1100 form for 0/1 bits.
   *
   * https://cdn-shop.adafruit.com/datasheets/SK6812.pdf
   */
   #define SK6812_I2S_LUT(x) (\
       (((x >> 0) & 0x1) ? 0b0000000000001100 : 0b0000000000001000) \
     | (((x >> 1) & 0x1) ? 0b0000000011000000 : 0b0000000010000000) \
     | (((x >> 2) & 0x1) ? 0b0000110000000000 : 0b0000100000000000) \
     | (((x >> 3) & 0x1) ? 0b1100000000000000 : 0b1000000000000000) \
   )

  static const uint16_t sk6812_i2s_lut[] = {
    [0b0000] = SK6812_I2S_LUT(0b0000),
    [0b0001] = SK6812_I2S_LUT(0b0001),
    [0b0010] = SK6812_I2S_LUT(0b0010),
    [0b0011] = SK6812_I2S_LUT(0b0011),
    [0b0100] = SK6812_I2S_LUT(0b0100),
    [0b0101] = SK6812_I2S_LUT(0b0101),
    [0b0110] = SK6812_I2S_LUT(0b0110),
    [0b0111] = SK6812_I2S_LUT(0b0111),
    [0b1000] = SK6812_I2S_LUT(0b1000),
    [0b1001] = SK6812_I2S_LUT(0b1001),
    [0b1010] = SK6812_I2S_LUT(0b1010),
    [0b1011] = SK6812_I2S_LUT(0b1011),
    [0b1100] = SK6812_I2S_LUT(0b1100),
    [0b1101] = SK6812_I2S_LUT(0b1101),
    [0b1110] = SK6812_I2S_LUT(0b1110),
    [0b1111] = SK6812_I2S_LUT(0b1111),
  };

  void leds_protocol_sk6812grbw_i2s_out(uint16_t buf[8], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union sk6812grbw_pixel pixel = sk6812grbw_pixel(pixels[index], index, limit);

    // 16-bit little-endian
    buf[0] = sk6812_i2s_lut[(pixel.grbw >> 28) & 0xf];
    buf[1] = sk6812_i2s_lut[(pixel.grbw >> 24) & 0xf];
    buf[2] = sk6812_i2s_lut[(pixel.grbw >> 20) & 0xf];
    buf[3] = sk6812_i2s_lut[(pixel.grbw >> 16) & 0xf];
    buf[4] = sk6812_i2s_lut[(pixel.grbw >> 12) & 0xf];
    buf[5] = sk6812_i2s_lut[(pixel.grbw >>  8) & 0xf];
    buf[6] = sk6812_i2s_lut[(pixel.grbw >>  4) & 0xf];
    buf[7] = sk6812_i2s_lut[(pixel.grbw >>  0) & 0xf];
  }
#endif
