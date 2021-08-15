#include "alert_led.h"

#include <logging.h>
#include <status_led.h>

struct alert_led_config {
  bool enabled;
  uint16_t gpio;
  bool inverted;
};

struct alert_led_config alert_led_config = {

};

const struct configtab alert_led_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &alert_led_config.enabled },
  },
  { CONFIG_TYPE_UINT16, "gpio",
    .uint16_type = { .value = &alert_led_config.gpio, .max = STATUS_LED_GPIO_MAX },
  },
  { CONFIG_TYPE_BOOL, "inverted",
    .bool_type = { .value = &alert_led_config.inverted },
  },
  {}
};

struct status_led *alert_led;

int init_alert_led()
{
  const struct status_led_options led_options = {
    .gpio     = alert_led_config.gpio,
    .inverted = alert_led_config.inverted,
  };
  const enum status_led_mode led_mode = STATUS_LED_OFF;

  if (!alert_led_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  LOG_INFO("enabled: gpio=%u inverted=%d", led_options.gpio, led_options.inverted);

  if (status_led_new(&alert_led, led_options, led_mode)) {
    LOG_ERROR("status_led_new");
    return -1;
  }

  return 0;
}
