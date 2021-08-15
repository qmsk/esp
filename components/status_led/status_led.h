#pragma once

#include <status_led.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

struct status_led {
  struct status_led_options options;

  xTaskHandle task;
  xQueueHandle queue;
  portTickType tick;

  enum status_led_mode mode;
  unsigned state;
};
