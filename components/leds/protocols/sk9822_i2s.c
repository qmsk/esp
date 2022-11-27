#include "sk9822.h"
#include "../leds.h"
#include "../interfaces/i2s.h"

#include <logging.h>

#if CONFIG_LEDS_I2S_ENABLED
  void leds_protocol_sk9822_i2s_out(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union sk9822_pixel pixel = sk9822_pixel(pixels[index], index, limit);

    // 32-bit little-endian
    buf[0] = pixel.rgbx;
  }
#endif
