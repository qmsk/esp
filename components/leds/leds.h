#pragma once

#include <leds.h>
#include <leds_status.h>

#include "interface.h"
#include "protocol.h"

#include <stdbool.h>
#include <stddef.h>

struct leds {
  struct leds_options options;

  // interface state
  union leds_interface_state interface;

  // protocol type
  const struct leds_protocol_type *protocol_type;

  // pixel state
  struct leds_color *pixels;

  // if false, all leds are inactive
  bool active;

  // limit used for leds_tx()
  struct leds_limit limit;

  struct leds_limit_status limit_total_status, *limit_groups_status;
};

int leds_init(struct leds *leds, const struct leds_options *options);

/* interface.c */
int leds_interface_init(union leds_interface_state *interface, const struct leds_protocol_type *protocol_type, const struct leds_options *options);

/* format.c */
void leds_set_format_rgb(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_bgr(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_grb(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_rgba(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_rgbw(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params);
