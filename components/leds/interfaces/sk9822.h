#pragma once

#include "../interface.h"
#include "../protocols/sk9822.h"

#if CONFIG_LEDS_I2S_ENABLED
  /* interfaces/i2s/sk9822.c */
  int leds_tx_i2s_sk9822(const struct leds_interface_i2s_options *options, union sk9822_pixel *pixels, unsigned count, struct leds_limit limit);
#endif
