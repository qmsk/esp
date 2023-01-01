#pragma once

#include "leds_state.h"
#include "leds_config.h"

#include <leds.h>

#include <stdbool.h>

struct leds_test_state {
  enum leds_test_mode mode;

  bool auto_mode;

  unsigned frame;
  TickType_t frame_tick;
};

int init_leds_test(struct leds_state *state, const struct leds_config *config);

/* Run next test mode */
void trigger_leds_test();

/* Keep running test modes automatically */
void auto_leds_test();

/* Clear test modes */
void reset_leds_test();

/* Clear test mode */
void leds_test_clear(struct leds_state *state);

/* Return next tick for test mode */
TickType_t leds_test_wait(struct leds_state *state);

/* Need update for test state? */
bool leds_test_active(struct leds_state *state, EventBits_t bits);

/* Update LEDs for test mode */
int leds_test_update(struct leds_state *state, EventBits_t bits);
