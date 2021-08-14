#include "activity_led.h"

#include <logging.h>
#include <status_led.h>

struct activity_led_config {
  bool enabled;
  uint16_t gpio;
  bool inverted;
};

struct activity_led_config activity_led_config = {

};

const struct configtab activity_led_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &activity_led_config.enabled },
  },
  { CONFIG_TYPE_UINT16, "gpio",
    .uint16_type = { .value = &activity_led_config.gpio },
  },
  { CONFIG_TYPE_BOOL, "inverted",
    .bool_type = { .value = &activity_led_config.inverted },
  },
  {}
};

struct status_led *activity_led;

int init_activity_led()
{
  const struct status_led_options led_options = {
    .gpio     = activity_led_config.gpio,
    .inverted = activity_led_config.inverted,
  };
  const enum status_led_mode led_mode = STATUS_LED_OFF;

  if (!activity_led_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  LOG_INFO("enabled: gpio=%u inverted=%d", led_options.gpio, led_options.inverted);

  if (status_led_new(&activity_led, led_options, led_mode)) {
    LOG_ERROR("status_led_new");
    return -1;
  }

  return 0;
}

void activity_led_event()
{
  if (!activity_led) {
    return;
  }

  if (status_led_mode(activity_led, STATUS_LED_FLASH)) {
    LOG_WARN("status_led_mode");
  }
}
