#pragma once

#include <user_leds.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// 100ms on / 1800ms off
#define USER_LEDS_SLOW_PERIOD_ON 100
#define USER_LEDS_SLOW_PERIOD_OFF 1800

// 100ms on / 100ms off
#define USER_LEDS_FAST_PERIOD 100

// 10ms on
#define USER_LEDS_FLASH_PERIOD 10

// 50ms off / 200ms on
#define USER_LEDS_PULSE_PERIOD_OFF 50
#define USER_LEDS_PULSE_PERIOD_ON 200

// 10ms read wait
#define USER_LEDS_READ_WAIT_PERIOD 10
#define USER_LEDS_READ_NOTIFY_BIT 0x1

#define USER_LEDS_EVENT_BITS ((1 << USER_LEDS_MAX) - 1)
#define USER_LEDS_EVENT_BIT(i) (1 << i)

struct user_leds_event {
  enum user_leds_state state;
};

struct user_led {
  struct user_leds_options options;

  SemaphoreHandle_t mutex;
  xQueueHandle queue;

  // set/override()
  enum user_leds_state set_state;
  bool set_override;

  // read()
  TaskHandle_t read_task;

  // schedule()
  portTickType tick; // next tick()

  // owned by task
  enum user_leds_state state, output_state;
  unsigned state_index;
};

struct user_leds {
  unsigned count;

  struct user_led *leds;

  // set()
  SemaphoreHandle_t mutex;
  EventGroupHandle_t event_group;
};

/* user_led.c */
int user_led_init(struct user_led *led, unsigned index, struct user_leds_options options);
TickType_t user_led_tick(struct user_led *led);
TickType_t user_led_schedule(struct user_led *led, TickType_t period);
void user_led_update(struct user_led *led);
