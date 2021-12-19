#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum pin_mutex {
  PIN_MUTEX_U0RXD,

  PIN_MUTEX_MAX,
};

extern SemaphoreHandle_t pin_mutex[PIN_MUTEX_MAX];

int init_pin_mutex();
