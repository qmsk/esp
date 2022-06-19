#pragma once

#include <leds.h>

#include <stdbool.h>

struct leds_test_state {
  enum leds_test_mode mode;

  bool auto_mode;
};

extern struct leds_test_state leds_test_state;

/* Run one test mode */
void trigger_leds_test();

/* Run test modes automatically */
void auto_leds_test();

/* Abort test modes */
void reset_leds_test();
