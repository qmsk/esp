#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum pin_mutex {
  PIN_MUTEX_U0RXD = 0,

  PIN_MUTEX_MAX,
};

/*
 * Note: These are actually binary semaphores created using `xSemaphoreCreateCounting()`, because it is not necessarily
 *       always the same task taking and giving the pin mutex. In particular, if the app_main() task takes the mutex,
 *       then a later `xSemaphoreGive()` will crash (!)
 */
extern SemaphoreHandle_t pin_mutex[PIN_MUTEX_MAX];

int init_pin_mutex();
