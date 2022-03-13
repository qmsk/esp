#pragma once

#include "../interface.h"
#include "../protocols/sk6812grbw.h"

#if CONFIG_LEDS_UART_ENABLED
  /* interfaces/uart/sk6812grbw.c */
  int leds_tx_uart_sk6812grbw(const struct leds_interface_uart_options *options, union sk6812grbw_pixel *pixels, unsigned count, struct leds_limit limit);
#endif

#if CONFIG_LEDS_I2S_ENABLED
  /* interfaces/i2s/sk6812grbw.c */
  int leds_tx_i2s_sk6812grbw(const struct leds_interface_i2s_options *options, union sk6812grbw_pixel *pixels, unsigned count, struct leds_limit limit);
#endif
