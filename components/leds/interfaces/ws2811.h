#pragma once

#include "../protocols/ws2811.h"

#if CONFIG_LEDS_UART_ENABLED
  /* interfaces/uart/ws2811.c */
  int leds_tx_uart_ws2811(const struct leds_options *options, union ws2811_pixel *pixels, unsigned count);
#endif

#if CONFIG_LEDS_I2S_ENABLED
  /* interfaces/i2s/ws2811.c */
  int leds_tx_i2s_ws2811(const struct leds_options *options, union ws2811_pixel *pixels, unsigned count);
#endif
