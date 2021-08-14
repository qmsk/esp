#include "atx_psu.h"

#include <driver/gpio.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#define ATX_PSU_GPIO_MAX GPIO_NUM_16

#define ATX_PSU_BIT(bit) (1 << bit)
#define ATX_PSU_BITS() ((1 << ATX_PSU_BIT_COUNT) - 1)
#define ATX_PSU_TIMEOUT 10 // s

#ifdef DEBUG
  // XXX: logging needs 1KB stack
  #define ATX_PSU_TASK_STACK 1024
#else
  #define ATX_PSU_TASK_STACK 256
#endif
#define ATX_PSU_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

struct atx_psu_config {
  bool enabled;
  uint16_t gpio;
  uint16_t timeout;
};

struct atx_psu_config atx_psu_config = {
  .timeout = ATX_PSU_TIMEOUT,
};

const struct configtab atx_psu_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &atx_psu_config.enabled },
  },
  { CONFIG_TYPE_UINT16, "gpio",
    .uint16_type = { .value = &atx_psu_config.gpio, .max = ATX_PSU_GPIO_MAX },
  },
  { CONFIG_TYPE_UINT16, "timeout",
    .uint16_type = { .value = &atx_psu_config.timeout },
  },
  {}
};

struct atx_psu {
  gpio_num_t gpio;

  xTaskHandle task;
  EventGroupHandle_t event_group;

} atx_psu;

static int init_atx_psu_gpio(struct atx_psu *atx_psu, const struct atx_psu_config *config)
{
  esp_err_t err;

  atx_psu->gpio = config->gpio;

  if ((err = gpio_set_direction(atx_psu->gpio, GPIO_MODE_OUTPUT))) {
    LOG_ERROR("gpio_set_direction: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = gpio_set_pull_mode(atx_psu->gpio, GPIO_PULLDOWN_ONLY))) {
    LOG_ERROR("gpio_set_pull_mode: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = gpio_set_level(atx_psu->gpio, 0))) {
    LOG_ERROR("gpio_set_level: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

static void atx_psu_set(bool active)
{
  esp_err_t err;

  LOG_DEBUG("%s", active ? "activate" : "deactivate");

  if ((err = gpio_set_level(atx_psu.gpio, active))) {
    // XXX: logging needs larger stack
    LOG_DEBUG("gpio_set_level: %s", esp_err_to_name(err));
  }
}

static EventBits_t atx_psu_wait(TickType_t ticks)
{
  LOG_DEBUG("ticks=%d", ticks);

  // wait for any bit except clear, dnot not clear
  return xEventGroupWaitBits(atx_psu.event_group, ATX_PSU_BITS() - ATX_PSU_BIT(ATX_PSU_BIT_CLEAR), pdFALSE, pdFALSE, ticks);
}

static EventBits_t atx_psu_wait_clear()
{
  LOG_DEBUG(" ");

  // wait for clear bit and clear it
  return xEventGroupWaitBits(atx_psu.event_group, ATX_PSU_BIT(ATX_PSU_BIT_CLEAR), pdTRUE, pdTRUE, portMAX_DELAY);
}

static void atx_psu_task(void *arg)
{
  TickType_t timeout = (atx_psu_config.timeout * 1000) / portTICK_PERIOD_MS;
  enum { INACTIVE, ACTIVE, DEACTIVATE } state = INACTIVE;

  for (;;) {
    switch(state) {
      case INACTIVE:
        LOG_DEBUG("INACTIVE...");

        // indefinite wait for any bit
        if (atx_psu_wait(portMAX_DELAY)) {
          LOG_DEBUG("INACTIVE -> ACTIVE");
          atx_psu_set(1);
          state = ACTIVE;
        } else {
          LOG_DEBUG("INACTIVE -> INACTIVE");
        }

        break;

      case ACTIVE:
        LOG_DEBUG("ACTIVE...");

        // indefinite wait for clear bit
        if (atx_psu_wait_clear()) {
          LOG_DEBUG("ACTIVE -> DEACTIVATE");
          // delay deactivation
          state = DEACTIVATE;
        } else {
          LOG_DEBUG("ACTIVE -> ACTIVE");
        }

        break;

      case DEACTIVATE:
        LOG_DEBUG("DEACTIVATE...");

        // wait for any bit or deactivate on timeout
        if (atx_psu_wait(timeout)) {
          LOG_DEBUG("DEACTIVATE -> ACTIVE");
          state = ACTIVE;
        } else {
          LOG_DEBUG("DEACTIVATE -> INACTIVE");
          atx_psu_set(0);
          state = INACTIVE;
        }

        break;

      default:
        abort();
    }
  }
}

static int init_atx_psu_task(struct atx_psu *atx_psu)
{
  if (!(atx_psu->event_group = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  if (xTaskCreate(&atx_psu_task, "atx-psu", ATX_PSU_TASK_STACK, atx_psu, ATX_PSU_TASK_PRIORITY, &atx_psu->task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  }

  return 0;
}

int init_atx_psu()
{
  if (!atx_psu_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  LOG_INFO("enabled: gpio=%u", atx_psu_config.gpio);

  if (init_atx_psu_gpio(&atx_psu, &atx_psu_config)) {
    LOG_ERROR("init_atx_psu_gpio");
    return -1;
  }

  if (init_atx_psu_task(&atx_psu)) {
    LOG_ERROR("init_atx_psu_task");
    return -1;
  }

  return 0;
}

void activate_atx_psu(enum atx_psu_bit bit)
{
  if (atx_psu.event_group) {
    xEventGroupSetBits(atx_psu.event_group, ATX_PSU_BIT(bit));
  }
}

void deactivate_atx_psu(enum atx_psu_bit bit)
{
  if (atx_psu.event_group) {
    xEventGroupClearBits(atx_psu.event_group, ATX_PSU_BIT(bit));
    xEventGroupSetBits(atx_psu.event_group, ATX_PSU_BIT(ATX_PSU_BIT_CLEAR));
  }
}
