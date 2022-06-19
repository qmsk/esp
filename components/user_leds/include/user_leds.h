#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>

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

};

enum user_leds_input_event {
  USER_LEDS_INPUT_PRESS,
  USER_LEDS_INPUT_HOLD,
  USER_LEDS_INPUT_RELEASE,
};

struct user_leds_input {
  unsigned index; /** user_leds index */

  enum user_leds_input_event event;

  /**
   * Number of ticks that button has been held for, up until release
   */
  TickType_t hold;

  /*
   * Tick when button was pressed/released.
   */
  TickType_t press, release;
};

struct user_leds_options {
  gpio_num_t gpio;

  enum user_leds_mode mode;

  // input mode
  xQueueHandle input_queue; // struct user_leds_input
  EventGroupHandle_t input_event_group;
  EventBits_t input_event_bits;
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
 * @return <0 on error, 0 on success.
 */
int user_leds_set(struct user_leds *leds, unsigned index, enum user_leds_state state);
