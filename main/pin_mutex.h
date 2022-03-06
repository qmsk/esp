#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <sdkconfig.h>

/*
 * Lock pins for use by different conflicting peripherials/functions.
 */
enum pin_mutex {
#if CONFIG_IDF_TARGET_ESP8266

  PIN_MUTEX_U0RXD,

  PIN_MUTEX_MAX,

  PIN_MUTEX_I2S0_DATA = PIN_MUTEX_U0RXD,
  PIN_MUTEX_GPIO3     = PIN_MUTEX_U0RXD,

#elif CONFIG_IDF_TARGET_ESP32

  PIN_MUTEX_U0RXD,

  PIN_MUTEX_MAX,

  PIN_MUTEX_GPIO3     = PIN_MUTEX_U0RXD,

#else
  #error unsupported IDF_TARGET
#endif
};

/*
 * Note: These are actually binary semaphores created using `xSemaphoreCreateCounting()`, because it is not necessarily
 *       always the same task taking and giving the pin mutex. In particular, if the app_main() task takes the mutex,
 *       then a later `xSemaphoreGive()` will crash (!)
 */
extern SemaphoreHandle_t pin_mutex[PIN_MUTEX_MAX];

int init_pin_mutex();
