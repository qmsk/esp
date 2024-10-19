#include <atx_psu.h>

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <stdlib.h>

#define ATX_PSU_STATE_CLEAR_BIT (1 << 0)
#define ATX_PSU_STATE_BIT(bit) (1 << (bit + 1))
#define ATX_PSU_STATE_BITS ((1 << ATX_PSU_BIT_COUNT) - 1 - 1) // all bits, but not the clear bit

#define ATX_PSU_STATUS_POWER_ENABLE_BIT (1 << 0)
#define ATX_PSU_STATUS_POWER_GOOD_BIT (1 << 1)

// poll for PG while ACTIVATE -> ACTIVE
#define ATX_PSU_POWER_GOOD_ACTIVATE_TICKS (100 / portTICK_PERIOD_MS)

// poll for PG while ACTIVATE
#define ATX_PSU_POWER_GOOD_ACTIVE_TICKS (1000 / portTICK_PERIOD_MS)

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

static IRAM_ATTR void atx_psu_gpio_interrupt(gpio_pins_t pins, void *arg)
{
  struct atx_psu *atx_psu = arg;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  LOG_ISR_DEBUG("pins=" GPIO_PINS_FMT, GPIO_PINS_ARGS(pins));

#if CONFIG_IDF_TARGET_ESP8266
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
#else
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
}

static int atx_psu_init(struct atx_psu *atx_psu, const struct atx_psu_options *options)
{
  int err;

  atx_psu->options = *options;

  // use interrupts for power_good input pins
  atx_psu->options.gpio_options.interrupt_pins = atx_psu->options.gpio_options.in_pins;
  atx_psu->options.gpio_options.interrupt_func = atx_psu_gpio_interrupt;
  atx_psu->options.gpio_options.interrupt_arg = atx_psu;

  if ((err = gpio_setup(&atx_psu->options.gpio_options))) {
    LOG_ERROR("gpio_setup");
    return err;
  }

  if (!(atx_psu->state = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  if (!(atx_psu->status = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  return 0;
}

int atx_psu_new(struct atx_psu **atx_psup, const struct atx_psu_options *options)
{
  struct atx_psu *atx_psu;
  int err = 0;

  if (!(atx_psu = calloc(1, sizeof(*atx_psu)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = atx_psu_init(atx_psu, options))) {
    LOG_ERROR("atx_psu_init");
    goto error;
  }

  *atx_psup = atx_psu;

  return 0;

error:
  free(atx_psu);

  return err;
}

static void set_power_enable(struct atx_psu *atx_psu)
{
  LOG_DEBUG("pins=" GPIO_PINS_FMT " -> high", GPIO_PINS_ARGS(atx_psu->options.gpio_options.out_pins));

  xEventGroupSetBits(atx_psu->status, ATX_PSU_STATUS_POWER_ENABLE_BIT);

  if (gpio_out_set_all(&atx_psu->options.gpio_options)) {
    LOG_WARN("gpio_out_set_all");
  }
}

static void clear_power_enable(struct atx_psu *atx_psu)
{
  LOG_DEBUG("pins=" GPIO_PINS_FMT " -> low", GPIO_PINS_ARGS(atx_psu->options.gpio_options.out_pins));

  xEventGroupClearBits(atx_psu->status, ATX_PSU_STATUS_POWER_ENABLE_BIT | ATX_PSU_STATUS_POWER_GOOD_BIT);

  if (gpio_out_clear(&atx_psu->options.gpio_options)) {
    LOG_WARN("gpio_out_clear");
  }
}

static bool get_power_good(struct atx_psu *atx_psu)
{
  gpio_pins_t pins;
  int err;

  if (!atx_psu->options.gpio_options.in_pins) {
    LOG_DEBUG("pins=" GPIO_PINS_FMT " unknown -> true", GPIO_PINS_ARGS(pins));
    xEventGroupSetBits(atx_psu->status, ATX_PSU_STATUS_POWER_GOOD_BIT);
    return true;
  } else if ((err = gpio_in_get(&atx_psu->options.gpio_options, &pins))) {
    LOG_DEBUG("pins=" GPIO_PINS_FMT " error -> true", GPIO_PINS_ARGS(pins));
    return true;
  } else if (pins) {
    LOG_DEBUG("pins=" GPIO_PINS_FMT " high -> true", GPIO_PINS_ARGS(pins));
    xEventGroupSetBits(atx_psu->status, ATX_PSU_STATUS_POWER_GOOD_BIT);
    return true;
  } else {
    LOG_DEBUG("pins=" GPIO_PINS_FMT " low -> false", GPIO_PINS_ARGS(pins));
    xEventGroupClearBits(atx_psu->status, ATX_PSU_STATUS_POWER_GOOD_BIT);
    return false;
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
          LOG_DEBUG("INACTIVE -> ACTIVATE<%08x>", bits);
          set_power_enable(atx_psu);
          state = ACTIVATE;
          LOG_INFO("power-on");
        } else {
          LOG_DEBUG("INACTIVE");
        }

        break;

      case ACTIVATE:
        LOG_DEBUG("ACTIVATE...");

        // poll for PG
        if (!(bits = atx_psu_wait_clear(atx_psu, ATX_PSU_POWER_GOOD_ACTIVATE_TICKS))) {
          LOG_DEBUG("ACTIVATE -> DEACTIVATE");
          // delay deactivation
          state = DEACTIVATE;
        } else if (get_power_good(atx_psu)) {
          LOG_DEBUG("ACTIVATE -> ACTIVE<%08x>", bits);
          state = ACTIVE;
          LOG_INFO("power-good");
        } else {
          // re-ACTIVATE to test power_good
          LOG_DEBUG("ACTIVATE");
        }

        break;

      case ACTIVE:
        LOG_DEBUG("ACTIVE...");

        // wait for clear bit, poll for PG
        if (!(bits = atx_psu_wait_clear(atx_psu, ATX_PSU_POWER_GOOD_ACTIVE_TICKS))) {
          LOG_DEBUG("ACTIVE -> DEACTIVATE");
          // delay deactivation
          state = DEACTIVATE;
        } else if (get_power_good(atx_psu)) {
          LOG_DEBUG("ACTIVE<%08x>", bits);
        } else {
          // re-ACTIVATE to test power_good
          LOG_DEBUG("ACTIVE -> ACTIVATE");
          state = ACTIVATE;
          LOG_WARN("lost power-good!");
        }

        break;

      case DEACTIVATE:
        LOG_DEBUG("DEACTIVATE...");

        // wait for any bit or deactivate on timeout
        if (!(bits = atx_psu_wait(atx_psu, atx_psu->options.timeout))) {
          LOG_DEBUG("DEACTIVATE -> INACTIVE");
          clear_power_enable(atx_psu);
          state = INACTIVE;
          LOG_INFO("power-off");
        } else if (get_power_good(atx_psu)) {
          LOG_DEBUG("DEACTIVATE -> ACTIVE<%08x>", bits);
          state = ACTIVE;
        } else {
          LOG_DEBUG("DEACTIVATE -> ACTIVATE<%08x>", bits);
          state = ACTIVATE;
        }

        break;

      default:
        abort();
    }
  }
}

void atx_psu_power_enable(struct atx_psu *atx_psu, enum atx_psu_bit bit)
{
  LOG_DEBUG("bit=%08x", ATX_PSU_STATE_BIT(bit));

  xEventGroupSetBits(atx_psu->state, ATX_PSU_STATE_BIT(bit));
}

int atx_psu_power_good(struct atx_psu *atx_psu, enum atx_psu_bit bit, TickType_t timeout)
{
  EventBits_t bits;

  xEventGroupSetBits(atx_psu->state, ATX_PSU_STATE_BIT(bit));

  if (!atx_psu->options.gpio_options.in_pins) {
    LOG_DEBUG("power_good_gpio disabled");

    return ATX_PSU_STATUS_POWER_GOOD_BIT;
  } else {
    bits = xEventGroupWaitBits(atx_psu->status, ATX_PSU_STATUS_POWER_ENABLE_BIT | ATX_PSU_STATUS_POWER_GOOD_BIT, pdFALSE, pdTRUE, timeout);

    //LOG_DEBUG("bits=%08x", bits);

    return (bits & ATX_PSU_STATUS_POWER_GOOD_BIT);
  }
}

void atx_psu_standby(struct atx_psu *atx_psu, enum atx_psu_bit bit)
{
  LOG_DEBUG("bit=%08x", ATX_PSU_STATE_BIT(bit));

  xEventGroupClearBits(atx_psu->state, ATX_PSU_STATE_BIT(bit));
  xEventGroupSetBits(atx_psu->state, ATX_PSU_STATE_CLEAR_BIT);
}
