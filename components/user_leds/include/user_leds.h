#pragma once

#include <freertos/FreeRTOS.h>

#include <driver/gpio.h>
#include <stdbool.h>

#define USER_LEDS_MAX 8

enum user_leds_mode {
  USER_LEDS_MODE_INPUT_BIT      = 0x01,
  USER_LEDS_MODE_OUTPUT_BIT     = 0x02,
  USER_LEDS_MODE_INVERTED_BIT   = 0x10,

  USER_LEDS_MODE_NONE             = 0,
  USER_LEDS_MODE_OUTPUT_HIGH      = (USER_LEDS_MODE_OUTPUT_BIT),
  USER_LEDS_MODE_OUTPUT_LOW       = (USER_LEDS_MODE_OUTPUT_BIT | USER_LEDS_MODE_INVERTED_BIT),
  USER_LEDS_MODE_INPUT_HIGH       = (USER_LEDS_MODE_INPUT_BIT),
  USER_LEDS_MODE_INPUT_LOW        = (USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_INVERTED_BIT),
  USER_LEDS_MODE_INPUTOUTPUT_HIGH = (USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_OUTPUT_BIT),
  USER_LEDS_MODE_INPUTOUTPUT_LOW  = (USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_OUTPUT_BIT | USER_LEDS_MODE_INVERTED_BIT),
};

enum user_leds_state {
  USER_LEDS_OFF,
  USER_LEDS_ON,
  USER_LEDS_SLOW,   // blink slow
  USER_LEDS_FAST,   // blink fast
  USER_LEDS_FLASH,  // blink once
  USER_LEDS_PULSE,  // pulse off

  USER_LEDS_READ_WAIT,
  USER_LEDS_READ,
};

struct user_leds_options {
  gpio_num_t gpio;
  enum user_leds_mode mode;
};

struct user_leds;

int user_leds_new(struct user_leds **ledsp, size_t count, const struct user_leds_options options[]);

/*
 * Run user_leds mainloop.
 *
 * @param arg `struct user_leds *`
 */
void user_leds_main(void *arg);

/*
 * Set output mode. Newest update wins.
 *
 * @return <0 on error, 0 on success, 1 on timeout
 */
int user_leds_set(struct user_leds *leds, unsigned index, enum user_leds_state state, TickType_t timeout);

/*
 * Override output mode. Supresses user_leds_set() until reverted.
 */
int user_leds_override(struct user_leds *leds, unsigned index, enum user_leds_state state);
int user_leds_revert(struct user_leds *leds, unsigned index);

/*
 * Briefly set GPIO as input, and read level set by external pull-up/down or switch.
 *
 * This will block concurrent user_leds_mode() changes for one tick during the read.
 * After the read, the output will remain floating.
 *
 * Returns <0 on error, 0 if idle, >0 if set.
 */
int user_leds_read(struct user_leds *leds, unsigned index);
