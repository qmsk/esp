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
  bool pixels_limit_dirty; // recalculate leds_limit_status

  // limit used for leds_tx()
  struct leds_limit limit;
  struct leds_limit_status limit_total_status, *limit_groups_status;
};


int leds_init(struct leds *leds, const struct leds_options *options);

/* interface.c */
int leds_interface_init(union leds_interface_state *interface, const struct leds_protocol_type *protocol_type, const struct leds_options *options);

/* color.c */
unsigned leds_colors_active (const struct leds_color *colors, unsigned count, enum leds_parameter_type parameter_type);

/* power.c */
unsigned leds_power_total(const struct leds_color *pixels, unsigned index, unsigned count, enum leds_power_mode power_mode);

/* limit.c */
void leds_limit_update(struct leds *leds);

/* format.c */
void leds_set_format_rgb(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_bgr(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_grb(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_rgba(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params);
void leds_set_format_rgbw(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params);
