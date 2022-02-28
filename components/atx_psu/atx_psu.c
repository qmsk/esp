#include <atx_psu.h>

#include <logging.h>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <stdlib.h>

#define ATX_PSU_STATE_CLEAR_BIT (1 << 0)
#define ATX_PSU_STATE_BIT(bit) (1 << (bit + 1))
#define ATX_PSU_STATE_BITS ((1 << ATX_PSU_BIT_COUNT) - 1 - 1) // all bits, but not the clear bit

#define ATX_PSU_STATUS_POWER_ENABLE_BIT (1 << 0)
#define ATX_PSU_STATUS_POWER_GOOD_BIT (1 << 1)

#define ATX_PSU_POWER_GOOD_TICKS (100 / portTICK_PERIOD_MS)

enum atx_psu_state {
  INACTIVE,
  ACTIVATE,
  ACTIVE,
  DEACTIVATE,
};

struct atx_psu {
  struct atx_psu_options options;

  EventGroupHandle_t state, status;

} atx_psu;

static int init_power_enable_gpio(struct atx_psu *atx_psu, struct atx_psu_options options)
{
  gpio_config_t config = {
    .pin_bit_mask = (1 << options.power_enable_gpio),
    .mode         = GPIO_MODE_OUTPUT,
    .pull_down_en = true,
  };
  esp_err_t err;

  LOG_DEBUG("gpio=%d", options.power_enable_gpio);

  if ((err = gpio_config(&config))) {
    LOG_ERROR("gpio_config: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

static int init_power_good_gpio(struct atx_psu *atx_psu, struct atx_psu_options options)
{
  gpio_config_t config = {
    .pin_bit_mask = (1 << options.power_good_gpio),
    .mode         = GPIO_MODE_INPUT,
    .pull_up_en   = true,
  };
  esp_err_t err;

  LOG_DEBUG("gpio=%d", options.power_good_gpio);

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

  if (!(atx_psu->state = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  if (!(atx_psu->status = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  if (options.power_enable_gpio < 0) {
    LOG_DEBUG("power_enable_gpio disabled");
  } else if (init_power_enable_gpio(atx_psu, options)) {
    LOG_ERROR("init_power_enable_gpio");
    return -1;
  }

  if (options.power_good_gpio < 0) {
    LOG_DEBUG("power_good_gpio disabled");
  } else if (init_power_good_gpio(atx_psu, options)) {
    LOG_ERROR("init_power_good_gpio");
    return -1;
  }

  *atx_psup = atx_psu;

  return 0;
}

static void set_power_enable(struct atx_psu *atx_psu)
{
  esp_err_t err;

  LOG_DEBUG("power_enable_gpio=%d -> true", atx_psu->options.power_enable_gpio);

  xEventGroupSetBits(atx_psu->status, ATX_PSU_STATUS_POWER_ENABLE_BIT);

  if (atx_psu->options.power_enable_gpio < 0) {

  } else if ((err = gpio_set_level(atx_psu->options.power_enable_gpio, true))) {
    LOG_WARN("gpio_set_level: %s", esp_err_to_name(err));
  }
}

static void clear_power_enable(struct atx_psu *atx_psu)
{
  esp_err_t err;

  LOG_DEBUG("power_enable_gpio=%d -> false", atx_psu->options.power_enable_gpio);

  xEventGroupClearBits(atx_psu->status, ATX_PSU_STATUS_POWER_ENABLE_BIT | ATX_PSU_STATUS_POWER_GOOD_BIT);

  if (atx_psu->options.power_enable_gpio < 0) {

  } else if ((err = gpio_set_level(atx_psu->options.power_enable_gpio, false))) {
    LOG_WARN("gpio_set_level: %s", esp_err_to_name(err));
  }
}

static bool get_power_good(struct atx_psu *atx_psu)
{
  if (atx_psu->options.power_good_gpio < 0) {
    LOG_DEBUG("no power_good_gpio -> true");
    xEventGroupSetBits(atx_psu->status, ATX_PSU_STATUS_POWER_GOOD_BIT);
    return true;
  } else if (gpio_get_level(atx_psu->options.power_good_gpio)) {
    LOG_DEBUG("power_good_gpio=%d high -> false", atx_psu->options.power_good_gpio);
    xEventGroupClearBits(atx_psu->status, ATX_PSU_STATUS_POWER_GOOD_BIT);
    return false; // inverting
  } else {
    LOG_DEBUG("power_good_gpio=%d low -> true", atx_psu->options.power_good_gpio);
    xEventGroupSetBits(atx_psu->status, ATX_PSU_STATUS_POWER_GOOD_BIT);
    return true; // inverting
  }
}

static EventBits_t atx_psu_wait(struct atx_psu *atx_psu, TickType_t ticks)
{
  LOG_DEBUG("ticks=%d", ticks);

  // wait for any bit except clear, do not clear
  return xEventGroupWaitBits(atx_psu->state, ATX_PSU_STATE_BITS, pdFALSE, pdFALSE, ticks) & ATX_PSU_STATE_BITS;
}

static EventBits_t atx_psu_wait_clear(struct atx_psu *atx_psu, TickType_t ticks)
{
  LOG_DEBUG("ticks=%d", ticks);

  // wait for clear bit and clear it
  return xEventGroupWaitBits(atx_psu->state, ATX_PSU_STATE_CLEAR_BIT, pdTRUE, pdTRUE, ticks) & ATX_PSU_STATE_BITS;
}

void atx_psu_main(void *arg)
{
  struct atx_psu *atx_psu = arg;
  enum atx_psu_state state = INACTIVE;

  for (;;) {
    EventBits_t bits;

    switch(state) {
      case INACTIVE:
        LOG_DEBUG("INACTIVE...");

        // indefinite wait for any bit
        if ((bits = atx_psu_wait(atx_psu, portMAX_DELAY))) {
          LOG_DEBUG("INACTIVE -> ACTIVE<%08x>", bits);
          set_power_enable(atx_psu);
          state = ACTIVATE;
        } else {
          LOG_DEBUG("INACTIVE -> ACTIVATE");
        }

        break;

      case ACTIVATE:
        LOG_DEBUG("ACTIVATE...");

        // indefinite wait for clear bit
        if (get_power_good(atx_psu)) {
          LOG_DEBUG("ACTIVATE -> ACTIVE");
          state = ACTIVE;
        } else if ((bits = atx_psu_wait_clear(atx_psu, ATX_PSU_POWER_GOOD_TICKS))) {
          // test power_good
        } else {
          LOG_DEBUG("ACTIVE -> DEACTIVATE");
          // delay deactivation
          state = DEACTIVATE;
        }

        break;

      case ACTIVE:
        LOG_DEBUG("ACTIVE...");

        // indefinite wait for clear bit
        if ((bits = atx_psu_wait_clear(atx_psu, portMAX_DELAY))) {
          LOG_DEBUG("ACTIVE -> ACTIVE<%08x>", bits);
        } else {
          LOG_DEBUG("ACTIVE -> DEACTIVATE");
          // delay deactivation
          state = DEACTIVATE;
        }

        break;

      case DEACTIVATE:
        LOG_DEBUG("DEACTIVATE...");

        // wait for any bit or deactivate on timeout
        if ((bits = atx_psu_wait(atx_psu, atx_psu->options.timeout))) {
          LOG_DEBUG("DEACTIVATE -> ACTIVE<%08x>", bits);
          state = ACTIVE;
        } else {
          LOG_DEBUG("DEACTIVATE -> INACTIVE");
          clear_power_enable(atx_psu);
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
  xEventGroupSetBits(atx_psu->state, ATX_PSU_STATE_BIT(bit));
}

int atx_psu_power_good(struct atx_psu *atx_psu, enum atx_psu_bit bit, TickType_t timeout)
{
  EventBits_t bits;

  xEventGroupSetBits(atx_psu->state, ATX_PSU_STATE_BIT(bit));

  if (atx_psu->options.power_good_gpio < 0) {
    LOG_DEBUG("power_good_gpio disabled");

    return ATX_PSU_STATUS_POWER_GOOD_BIT;
  } else {
    bits = xEventGroupWaitBits(atx_psu->status, ATX_PSU_STATUS_POWER_ENABLE_BIT | ATX_PSU_STATUS_POWER_GOOD_BIT, pdFALSE, pdTRUE, timeout);

    LOG_DEBUG("bits=%08x", bits);

    return (bits & ATX_PSU_STATUS_POWER_GOOD_BIT);
  }
}

void atx_psu_standby(struct atx_psu *atx_psu, enum atx_psu_bit bit)
{
  xEventGroupClearBits(atx_psu->state, ATX_PSU_STATE_BIT(bit));
  xEventGroupSetBits(atx_psu->state, ATX_PSU_STATE_CLEAR_BIT);
}
