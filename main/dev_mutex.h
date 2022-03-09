#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <sdkconfig.h>

/*
 * Lock peripheral devices used by different components.
 */
enum dev_mutex {
#if CONFIG_IDF_TARGET_ESP8266

  DEV_MUTEX_UART0,

  DEV_MUTEX_MAX,

#elif CONFIG_IDF_TARGET_ESP32

  DEV_MUTEX_UART0,

  DEV_MUTEX_MAX,

#else
  #error unsupported IDF_TARGET
#endif
};

/*
 * Note: These are actually binary semaphores created using `xSemaphoreCreateCounting()`, because it is not necessarily
 *       always the same task taking and giving the pin mutex. In particular, if the app_main() task takes the mutex,
 *       then a later `xSemaphoreGive()` will crash (!)
 */
extern SemaphoreHandle_t dev_mutex[DEV_MUTEX_MAX];

int init_dev_mutex();
