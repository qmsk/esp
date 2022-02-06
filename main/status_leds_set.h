#pragma once

#include <status_led.h>

#include <sdkconfig.h>

#if CONFIG_STATUS_LEDS_USER_ENABLED
  void set_user_led(enum status_led_mode mode);
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  void set_flash_led(enum status_led_mode mode);
#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  void set_alert_led(enum status_led_mode mode);
#endif
