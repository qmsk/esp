#pragma once

#include <user_leds.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

enum user_leds_op {
  USER_LEDS_SET,
  USER_LEDS_READ,
};

struct user_leds_event {
  enum user_leds_op op;
  enum user_leds_mode mode;
};

struct user_leds {
  struct user_leds_options options;

  // operations
  SemaphoreHandle_t mutex;
  xQueueHandle queue;
  TaskHandle_t read_task;
  bool override;
  enum user_leds_mode set_mode;

  // owned by task
  portTickType tick;
  enum user_leds_mode mode;
  unsigned state;
  bool input, input_wait;
};
