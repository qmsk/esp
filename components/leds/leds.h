 #pragma once

#include <leds.h>

#if CONFIG_LEDS_SPI_ENABLED
# include "interfaces/spi.h"
#endif

union leds_interface_state {
  #if CONFIG_LEDS_SPI_ENABLED
      struct leds_interface_spi spi;
  #endif
};

#include "apa102.h"
#include "p9813.h"
#include "sk6812grbw.h"
#include "ws2811.h"
#include "ws2812b.h"

#include <stdbool.h>
#include <stddef.h>

struct leds {
  struct leds_options options;

  // interface state
  union leds_interface_state interface;

  // protocol state
  union {
    struct leds_protocol_apa102 apa102;
    struct leds_protocol_p9813 p9813;
    struct leds_protocol_ws2812b ws2812b;
    struct leds_protocol_sk6812grbw sk6812grbw;
    struct leds_protocol_ws2811 ws2811;
  } state;

  // if false, all leds are inactive
  bool active;
};

int leds_init(struct leds *leds, const struct leds_options *options);

/* format.c */
void leds_set_format_rgb(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_bgr(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_grb(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_rgba(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_rgbw(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
