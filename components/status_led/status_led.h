#pragma once

#include <status_led.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

enum status_led_op {
  STATUS_LED_SET,
  STATUS_LED_READ,
};

struct status_led_event {
  enum status_led_op op;
  enum status_led_mode mode;
};

struct status_led {
  struct status_led_options options;

  // operations
  SemaphoreHandle_t mutex;
  xQueueHandle queue;
  TaskHandle_t read_task;
  bool override;
  enum status_led_mode set_mode;

  // owned by task
  portTickType tick;
  enum status_led_mode mode;
  unsigned state;
  bool input, input_wait;
};
