#ifndef __USER_LED_H__
#define __USER_LED_H__

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <drivers/gpio.h>

/* Status LED */
struct led_options {
  enum GPIO gpio;

  bool inverted;
};

enum led_mode {
    LED_OFF,
    LED_ON,
    LED_SLOW,   // blink slow
    LED_FAST,   // blink fast
    LED_BLINK,  // blink once
};

struct led {
  struct led_options options;

  xTaskHandle task;
  xQueueHandle queue;
  portTickType tick;

  enum led_mode mode;
  unsigned state;
};

int led_init(struct led *led, const struct led_options options, enum led_mode mode);
int led_set(struct led *led, enum led_mode mode);

/* User LED */
int init_led(enum led_mode mode);
int set_led(enum led_mode mode);

#endif
