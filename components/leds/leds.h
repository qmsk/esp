#pragma once

#include <leds.h>
#include <leds_stats.h>

#include "interface.h"
#if CONFIG_LEDS_SPI_ENABLED
# include "interfaces/spi.h"
#endif

#include "protocol.h"
#include "protocols/apa102.h"
#include "protocols/p9813.h"
#include "protocols/sk6812grbw.h"
#include "protocols/ws2811.h"
#include "protocols/ws2812b.h"
#include "protocols/sk9822.h"

union leds_interface_state {
#if CONFIG_LEDS_SPI_ENABLED
  struct leds_interface_spi spi;
#endif
};

union leds_protocol_state {
  struct leds_protocol_apa102 apa102;
  struct leds_protocol_p9813 p9813;
  struct leds_protocol_ws2812b ws2812b;
  struct leds_protocol_sk6812grbw sk6812grbw;
  struct leds_protocol_ws2811 ws2811;
  struct leds_protocol_sk9822 sk9822;
};

#include <stdbool.h>
#include <stddef.h>

struct leds {
  struct leds_options options;

  // interface state
  union leds_interface_state interface;

  // protocol state
  union leds_protocol_state protocol;

  // if false, all leds are inactive
  bool active;

  // limit used for leds_tx()
  struct leds_limit limit;

  struct leds_limit_stats limit_total_stats, *limit_groups_stats;
};

int leds_init(struct leds *leds, const struct leds_options *options);

/* format.c */
void leds_set_format_rgb(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_bgr(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_grb(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_rgba(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_rgbw(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
