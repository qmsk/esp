#include <atx_psu.h>

#include <logging.h>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <stdlib.h>

#define ATX_PSU_BIT(bit) (1 << bit)
#define ATX_PSU_BITS() ((1 << ATX_PSU_BIT_COUNT) - 1)
#define ATX_PSU_TIMEOUT 10 // s

struct atx_psu {
  struct atx_psu_options options;

  EventGroupHandle_t event_group;

} atx_psu;

int atx_psu_init_gpio(struct atx_psu *atx_psu, struct atx_psu_options options)
{
  gpio_config_t config = {
    .pin_bit_mask = (1 << options.gpio),
    .mode         = GPIO_MODE_OUTPUT,
    .pull_down_en = true,
  };
  esp_err_t err;

  LOG_DEBUG("gpio=%d", options.gpio);

  if ((err = gpio_config(&config))) {
    LOG_ERROR("gpio_config: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int atx_psu_new(struct atx_psu **atx_psup, struct atx_psu_options options)
{
  struct atx_psu *atx_psu;

  if (!(atx_psu = calloc(1, sizeof(*atx_psu)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  atx_psu->options = options;

  if (!(atx_psu->event_group = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  if (atx_psu_init_gpio(atx_psu, options)) {
    LOG_ERROR("atx_psu_init_gpio");
    return -1;
  }

  *atx_psup = atx_psu;

  return 0;
}

static void atx_psu_set(struct atx_psu *atx_psu, bool active)
{
  esp_err_t err;

  LOG_DEBUG("%s", active ? "active" : "standby");

  if ((err = gpio_set_level(atx_psu->options.gpio, active))) {
    LOG_WARN("gpio_set_level: %s", esp_err_to_name(err));
  }
}

static EventBits_t atx_psu_wait(struct atx_psu *atx_psu, TickType_t ticks)
{
  LOG_DEBUG("ticks=%d", ticks);

  // wait for any bit except clear, dnot not clear
  return xEventGroupWaitBits(atx_psu->event_group, ATX_PSU_BITS() - ATX_PSU_BIT(ATX_PSU_BIT_CLEAR), pdFALSE, pdFALSE, ticks);
}

static EventBits_t atx_psu_wait_clear(struct atx_psu *atx_psu, TickType_t ticks)
{
  LOG_DEBUG(" ");

  // wait for clear bit and clear it
  return xEventGroupWaitBits(atx_psu->event_group, ATX_PSU_BIT(ATX_PSU_BIT_CLEAR), pdTRUE, pdTRUE, ticks);
}

void atx_psu_main(void *arg)
{
  struct atx_psu *atx_psu = arg;
  enum { INACTIVE, ACTIVE, DEACTIVATE } state = INACTIVE;

  for (;;) {
    switch(state) {
      case INACTIVE:
        LOG_DEBUG("INACTIVE...");

        // indefinite wait for any bit
        if (atx_psu_wait(atx_psu, portMAX_DELAY)) {
          LOG_DEBUG("INACTIVE -> ACTIVE");
          atx_psu_set(atx_psu, true);
          state = ACTIVE;
        } else {
          LOG_DEBUG("INACTIVE -> INACTIVE");
        }

        break;

      case ACTIVE:
        LOG_DEBUG("ACTIVE...");

        // indefinite wait for clear bit
        if (atx_psu_wait_clear(atx_psu, portMAX_DELAY)) {
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
        if (atx_psu_wait(atx_psu, atx_psu->options.timeout)) {
          LOG_DEBUG("DEACTIVATE -> ACTIVE");
          state = ACTIVE;
        } else {
          LOG_DEBUG("DEACTIVATE -> INACTIVE");
          atx_psu_set(atx_psu, false);
          state = INACTIVE;
        }

        break;

      default:
        abort();
    }
  }
}

void atx_psu_power_enable(struct atx_psu *atx_psu, enum atx_psu_bit bit)
{
  xEventGroupSetBits(atx_psu->event_group, ATX_PSU_BIT(bit));
}

void atx_psu_standby(struct atx_psu *atx_psu, enum atx_psu_bit bit)
{
  xEventGroupClearBits(atx_psu->event_group, ATX_PSU_BIT(bit));
  xEventGroupSetBits(atx_psu->event_group, ATX_PSU_BIT(ATX_PSU_BIT_CLEAR));
}
