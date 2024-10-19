#include "atx_psu.h"
#include "atx_psu_config.h"
#include "atx_psu_gpio.h"
#include "atx_psu_state.h"
#include "user.h"
#include "tasks.h"

#include <atx_psu.h>
#include <logging.h>

struct atx_psu *atx_psu;
xTaskHandle atx_psu_task;

int init_atx_psu()
{
  int err;

  struct atx_psu_options options = {
    .timeout  = (atx_psu_config.timeout * 1000) / portTICK_PERIOD_MS,
  };

  if (!atx_psu_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = config_atx_psu_gpio(&atx_psu_config, &options.gpio_options))) {
    LOG_ERROR("config_atx_psu_gpio");
    return err;
  }

  LOG_INFO("timeout=%u", options.timeout);

  if (atx_psu_new(&atx_psu, &options)) {
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
    LOG_WARN("power_good timeout=%dms expired, is ATX-PSU connected?", timeout * portTICK_PERIOD_MS);
    user_alert(USER_ALERT_ERROR_ATX_PSU_TIMEOUT); // TODO: clear alert?
    return -1;
  }
}

void clear_atx_psu_bit(enum atx_psu_bit bit)
{
  if (atx_psu) {
    atx_psu_standby(atx_psu, bit);
  }
}
