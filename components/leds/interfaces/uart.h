#pragma once

#include "../interface.h"
#include "../protocols/sk6812grbw.h"
#include "../protocols/ws2811.h"
#include "../protocols/ws2812b.h"

#if CONFIG_LEDS_UART_ENABLED
  /* interfaces/uart/sk6812grbw.c */
  int leds_tx_uart_sk6812grbw(const struct leds_interface_uart_options *options, union sk6812grbw_pixel *pixels, unsigned count, struct leds_limit limit);

  /* interfaces/uart/ws2811.c */
  int leds_tx_uart_ws2811(const struct leds_interface_uart_options *options, union ws2811_pixel *pixels, unsigned count, struct leds_limit limit);

  /* interfaces/uart/ws2812b.c */
  int leds_tx_uart_ws2812b(const struct leds_interface_uart_options *options, union ws2812b_pixel *pixels, unsigned count, struct leds_limit limit);
#endif
