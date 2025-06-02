#pragma once

#include <leds.h>
#include "../protocol.h"

extern struct leds_protocol_type leds_protocol_sm16703;

#if CONFIG_LEDS_I2S_ENABLED
  #include "../interfaces/i2s.h"

  void leds_protocol_sm16703_i2s_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
