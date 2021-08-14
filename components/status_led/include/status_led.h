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
};

struct status_led;

int status_led_new(struct status_led **ledp, const struct status_led_options options, enum status_led_mode mode);
int status_led_mode(struct status_led *led, enum status_led_mode mode);

#endif
