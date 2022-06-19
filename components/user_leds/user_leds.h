#pragma once

#include <user_leds.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// 100ms on / 1800ms off
#define USER_LEDS_SLOW_TICKS_ON (100 / portTICK_RATE_MS)
#define USER_LEDS_SLOW_TICKS_OFF (1800 / portTICK_RATE_MS)

// 100ms on / 100ms off
#define USER_LEDS_FAST_TICKS (100 / portTICK_RATE_MS)

// 10ms on
#define USER_LEDS_FLASH_TICKS (10 / portTICK_RATE_MS)

// 50ms off / 200ms on
#define USER_LEDS_PULSE_TICKS_OFF (50 / portTICK_RATE_MS)
#define USER_LEDS_PULSE_TICKS_ON (200 / portTICK_RATE_MS)

// read input every 1s
#define USER_LEDS_READ_IDLE_TICKS (1000 / portTICK_RATE_MS)
#define USER_LEDS_READ_HOLD_TICKS (500 / portTICK_RATE_MS)

// 10ms read wait
#define USER_LEDS_READ_WAIT_TICKS (10 / portTICK_RATE_MS)

#define USER_LEDS_EVENT_BITS ((1 << USER_LEDS_MAX) - 1)
#define USER_LEDS_EVENT_BIT(i) (1 << i)

enum user_leds_input_state {
  USER_LEDS_READ_IDLE,
  USER_LEDS_READ_WAIT,
  USER_LEDS_READ,
};

struct user_led {
  unsigned index;
  struct user_leds_options options;

  // set() updates
  xQueueHandle queue;

  // owned by task
  enum user_leds_state output_state;
  unsigned output_state_index;
  TickType_t output_tick; // next scheduled tick()

  // input
  enum user_leds_input_state input_state;
  TickType_t input_state_tick;
  TickType_t input_tick; // next scheduled input_tick()

  // interrupts
  EventGroupHandle_t leds_event_group;
  SemaphoreHandle_t input_interrupt;
};

struct user_leds {
  unsigned count;

  struct user_led *leds;

  // set() updates
  EventGroupHandle_t event_group;
};

/* user_led.c */
int user_led_init(struct user_led *led, unsigned index, struct user_leds_options options, EventGroupHandle_t leds_event_group);

TickType_t user_led_input_tick(struct user_led *led);
TickType_t user_led_input_schedule(struct user_led *led, TickType_t period);

TickType_t user_led_output_tick(struct user_led *led);
TickType_t user_led_output_schedule(struct user_led *led, TickType_t period);

void user_led_update(struct user_led *led);
