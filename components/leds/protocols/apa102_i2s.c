#include "apa102.h"
#include "../leds.h"
#include "../interfaces/i2s.h"

#include <logging.h>

#if CONFIG_LEDS_I2S_ENABLED
  void leds_protocol_apa102_i2s_out(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union apa102_pixel pixel = apa102_pixel(pixels[index], index, limit);

    // 32-bit little-endian
    buf[0] = pixel.rgbx;
  }
#endif
