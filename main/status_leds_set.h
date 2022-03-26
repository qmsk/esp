#pragma once

#include <status_led.h>

#include <sdkconfig.h>

#if CONFIG_STATUS_LEDS_USER_ENABLED
  int set_user_led(enum status_led_mode mode, TickType_t timeout);
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  int set_flash_led(enum status_led_mode mode, TickType_t timeout);
#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  int set_alert_led(enum status_led_mode mode, TickType_t timeout);
#endif
