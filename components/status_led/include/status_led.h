#ifndef __STATUS_LED_H__
#define __STATUS_LED_H__

#include <driver/gpio.h>
#include <stdbool.h>

#define STATUS_LED_GPIO_MAX GPIO_NUM_16

struct status_led_options {
  gpio_num_t gpio;

  bool inverted;
};

enum status_led_mode {
    STATUS_LED_OFF,
    STATUS_LED_ON,
    STATUS_LED_SLOW,   // blink slow
    STATUS_LED_FAST,   // blink fast
    STATUS_LED_FLASH,  // blink once
    STATUS_LED_PULSE,  // pulse off
};

struct status_led;

int status_led_new(struct status_led **ledp, const struct status_led_options options, enum status_led_mode mode);

/*
 * Set output mode. Newest update wins.
 */
int status_led_set(struct status_led *led, enum status_led_mode mode);

/*
 * Override output mode. Supresses status_led_set() until reverted.
 */
int status_leds_override(struct status_led *led, enum status_led_mode mode);
int status_leds_revert(struct status_led *led);

/*
 * Briefly set GPIO as input, and read level set by external pull-up/down or switch.
 *
 * This will block concurrent status_led_mode() changes for one tick during the read.
 * After the read, the output will remain floating.
 *
 * Returns <0 on error, 0 if idle, >0 if set.
 */
int status_led_read(struct status_led *led);

#endif
