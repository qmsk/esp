#pragma once

#include <config.h>
#include <sdkconfig.h>

enum user_led {
#if CONFIG_STATUS_LEDS_USER_ENABLED
  USER_LED,
#endif
#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  FLASH_LED,
#endif
#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  ALERT_LED,
#endif
#if CONFIG_STATUS_LEDS_CONFIG_ENABLED
  CONFIG_BUTTON,
#endif
#if CONFIG_STATUS_LEDS_TEST_ENABLED
  TEST_BUTTON,
#endif

  USER_LEDS_COUNT
};
