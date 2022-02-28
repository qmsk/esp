#include "atx_psu.h"
#include "atx_psu_state.h"
#include "tasks.h"

#include <atx_psu.h>
#include <logging.h>

#define ATX_PSU_TIMEOUT_DEFAULT 10

struct atx_psu_config {
  bool enabled;
  uint16_t power_enable_gpio;
  uint16_t power_good_gpio;
  uint16_t timeout;
};

struct atx_psu_config atx_psu_config = {

};

const struct configtab atx_psu_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &atx_psu_config.enabled },
  },
  { CONFIG_TYPE_UINT16, "power_enable_gpio",
    .alias = "gpio",
    .uint16_type = { .value = &atx_psu_config.power_enable_gpio, .max = (GPIO_NUM_MAX - 1) },
  },
  { CONFIG_TYPE_UINT16, "power_good_gpio",
    .uint16_type = { .value = &atx_psu_config.power_good_gpio, .max = (GPIO_NUM_MAX - 1) },
  },
  { CONFIG_TYPE_UINT16, "timeout",
    .description = "Power off ATX PSU after timeout seconds of idle",
    .uint16_type = { .value = &atx_psu_config.timeout, .default_value = ATX_PSU_TIMEOUT_DEFAULT },
  },
  {}
};

struct atx_psu *atx_psu;
xTaskHandle atx_psu_task;

int init_atx_psu()
{
  struct atx_psu_options options = {
    .power_enable_gpio  = atx_psu_config.power_enable_gpio ? atx_psu_config.power_enable_gpio : GPIO_NUM_NC,
    .power_good_gpio    = atx_psu_config.power_good_gpio ? atx_psu_config.power_good_gpio : GPIO_NUM_NC,

    .timeout  = (atx_psu_config.timeout * 1000) / portTICK_PERIOD_MS,
  };

  if (!atx_psu_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  LOG_INFO("power_enable_gpio=%u power_good_gpio=%u timeout=%u", options.power_enable_gpio, options.power_good_gpio, options.timeout);

  if (atx_psu_new(&atx_psu, options)) {
    LOG_ERROR("atx_psu_new");
    return -1;
  }

  return 0;
}

int start_atx_psu()
{
  struct task_options task_options = {
    .main       = atx_psu_main,
    .name       = ATX_PSU_TASK_NAME,
    .stack_size = ATX_PSU_TASK_STACK,
    .arg        = atx_psu,
    .priority   = ATX_PSU_TASK_PRIORITY,
    .handle     = &atx_psu_task,
    .affinity   = ATX_PSU_TASK_AFFINITY,
  };
  int err;

  if (!atx_psu) {
    return 0;
  }

  if ((err = start_task(task_options))) {
    LOG_ERROR("start_task");
    return err;
  }

  return 0;
}

void set_atx_psu_bit(enum atx_psu_bit bit)
{
  if (atx_psu) {
    atx_psu_power_enable(atx_psu, bit);
  }
}

int wait_atx_psu_bit(enum atx_psu_bit bit, TickType_t timeout)
{
  if (!atx_psu) {
    return 1;
  } else if (atx_psu_power_good(atx_psu, bit, timeout)) {
    return 0;
  } else {
    LOG_WARN("power_good timeout=%d expired, is ATX-PSU connected?", timeout);
    return -1;
  }
}

void clear_atx_psu_bit(enum atx_psu_bit bit)
{
  if (atx_psu) {
    atx_psu_standby(atx_psu, bit);
  }
}
