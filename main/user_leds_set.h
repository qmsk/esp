#pragma once

#include <user_leds.h>

#include <sdkconfig.h>

#if CONFIG_STATUS_LEDS_USER_ENABLED
  int set_user_led(enum user_leds_state state, TickType_t timeout);
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  int set_flash_led(enum user_leds_state state, TickType_t timeout);
#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  int set_alert_led(enum user_leds_state state, TickType_t timeout);
#endif
