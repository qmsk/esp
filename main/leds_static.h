#pragma once

#include "leds_state.h"

int config_leds_static(struct leds_state *state, const struct leds_config *config);


/* Return next tick */
TickType_t leds_static_wait(struct leds_state *state);

/* Need update? */
bool leds_static_active(struct leds_state *state, EventBits_t bits);

/* Update LEDs */
int leds_static_update(struct leds_state *state, EventBits_t bits);
