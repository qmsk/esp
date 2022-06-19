#pragma once

#include <freertos/FreeRTOS.h>

#include <driver/gpio.h>
#include <stdbool.h>

#define USER_LEDS_GPIO_MAX GPIO_NUM_16

struct user_leds_options {
  gpio_num_t gpio;

  bool inverted;
};

enum user_leds_mode {
    USER_LEDS_OFF,
    USER_LEDS_ON,
    USER_LEDS_SLOW,   // blink slow
    USER_LEDS_FAST,   // blink fast
    USER_LEDS_FLASH,  // blink once
    USER_LEDS_PULSE,  // pulse off
};

struct user_leds;

int user_leds_new(struct user_leds **ledsp, const struct user_leds_options options, enum user_leds_mode mode);

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
int user_leds_set(struct user_leds *leds, enum user_leds_mode mode, TickType_t timeout);

/*
 * Override output mode. Supresses user_leds_set() until reverted.
 */
int user_ledss_override(struct user_leds *leds, enum user_leds_mode mode);
int user_ledss_revert(struct user_leds *leds);

/*
 * Briefly set GPIO as input, and read level set by external pull-up/down or switch.
 *
 * This will block concurrent user_leds_mode() changes for one tick during the read.
 * After the read, the output will remain floating.
 *
 * Returns <0 on error, 0 if idle, >0 if set.
 */
int user_leds_read(struct user_leds *leds);
