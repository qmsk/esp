#include <atx_psu.h>

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <stdlib.h>

#define ATX_PSU_BIT(bit) (1 << (bit))
#define ATX_PSU_BITS ((1 << ATX_PSU_BIT_COUNT) - 1)

#define ATX_PSU_EVENTS_SET_BIT (1 << 0)
#define ATX_PSU_EVENTS_CLEAR_BIT (1 << 1)
#define ATX_PSU_EVENTS_INTR_BIT (1 << 2)

#define ATX_PSU_STATUS_POWER_ENABLE_BIT (1 << 0)
#define ATX_PSU_STATUS_POWER_GOOD_BIT (1 << 1)

// polling for PG
#define ATX_PSU_INACTIVE_TICKS (10000 / portTICK_PERIOD_MS)
#define ATX_PSU_ACTIVATE_TICKS (100 / portTICK_PERIOD_MS)
#define ATX_PSU_ACTIVE_TICKS (1000 / portTICK_PERIOD_MS)

enum atx_psu_state {
  INACTIVE,         // waiting for any bit set
  ACTIVATE,         // waiting for power-good, or bits cleared
  ACTIVE,           // waiting for bits cleared, checking for power-bad
  DEACTIVATE,       // waiting for delayed power-off, or any bit set
};

static const char *atx_psu_state_str(enum atx_psu_state state)
{
  switch (state) {
    case INACTIVE:    return "INACTIVE";
    case ACTIVATE:    return "ACTIVATE";
    case ACTIVE:      return "ACTIVE";
    case DEACTIVATE:  return "DEACTIVATE";
    default:          return "?";
  }
}

struct atx_psu {
  struct atx_psu_options options;
  enum atx_psu_state state;
  TickType_t tick;

  EventGroupHandle_t events;

  // XXX: these are just atomic bitmasks
  EventGroupHandle_t bits, status;

} atx_psu;

static IRAM_ATTR void atx_psu_gpio_interrupt(gpio_pins_t pins, void *arg)
{
  struct atx_psu *atx_psu = arg;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  LOG_ISR_DEBUG("pins=" GPIO_PINS_FMT, GPIO_PINS_ARGS(pins));

  if (!xEventGroupSetBitsFromISR(atx_psu->events, ATX_PSU_EVENTS_INTR_BIT, &xHigherPriorityTaskWoken)) {
    LOG_ISR_WARN("xEventGroupSetBitsFromISR");
  }

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

  if (!(atx_psu->events = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  if (!(atx_psu->bits = xEventGroupCreate())) {
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

static void next_state(struct atx_psu *atx_psu, enum atx_psu_state state, TickType_t ticks)
{
  EventBits_t bits = xEventGroupGetBits(atx_psu->bits);

  LOG_DEBUG("%s -> %s<%08x> @ %d", atx_psu_state_str(atx_psu->state), atx_psu_state_str(state), bits, ticks);

  atx_psu->state = state;
  atx_psu->tick = (ticks == portMAX_DELAY) ? 0 : xTaskGetTickCount() + ticks;
}

static EventBits_t atx_psu_wait(struct atx_psu *atx_psu)
{
  EventBits_t events = 0;

  // always follow input interrupts
  events |= ATX_PSU_EVENTS_INTR_BIT;

  switch(atx_psu->state) {
    case INACTIVE:
      // indefinite wait for any bit to be set
      events |= ATX_PSU_EVENTS_SET_BIT;
      break;

    case ACTIVATE:
      // fast wait for power-good
      events |= ATX_PSU_EVENTS_CLEAR_BIT;
      break;

    case ACTIVE:
      // slow wait for power-bad
      events |= ATX_PSU_EVENTS_CLEAR_BIT;
      break;

    case DEACTIVATE:
      // configurable wait for bits to remain cleared
      events |= ATX_PSU_EVENTS_SET_BIT;
      break;

    default:
      LOG_FATAL("state=%d", atx_psu->state);
  }

  // schedule
  TickType_t ticks, tick = xTaskGetTickCount();

  if (!atx_psu->tick) {
    ticks = portMAX_DELAY; // indefinite
  } else if (atx_psu->tick > tick) {
    ticks = atx_psu->tick - tick;
  } else {
    ticks = 0; // immediate
  }

  LOG_DEBUG("state=%s events=%04x ticks=%d", atx_psu_state_str(atx_psu->state), events, ticks);

  // wait for any bit and clear
  const BaseType_t xClearOnExit = pdTRUE;
  const BaseType_t xWaitForAllBits = pdFALSE;

  events = xEventGroupWaitBits(atx_psu->events, events, xClearOnExit, xWaitForAllBits, ticks);

  // check state tick timeout
  if (!atx_psu->tick) {
    return false;
  } else if (xTaskGetTickCount() >= atx_psu->tick) {
    return true;
  } else {
    return false;
  }
}

void atx_psu_main(void *arg)
{
  struct atx_psu *atx_psu = arg;

  for (;;) {
    bool timeout = atx_psu_wait(atx_psu);
    EventBits_t bits = xEventGroupGetBits(atx_psu->bits);
    bool power_good = get_power_good(atx_psu);

    LOG_DEBUG("state=%s bits=%04x power_good=%d timeout=%d", atx_psu_state_str(atx_psu->state), bits, power_good, timeout);

    switch(atx_psu->state) {
      case INACTIVE:
        if (bits) {
          next_state(atx_psu, ACTIVATE, ATX_PSU_ACTIVATE_TICKS);

          LOG_INFO("power-on");
          set_power_enable(atx_psu);

        } else {
          next_state(atx_psu, INACTIVE, ATX_PSU_INACTIVE_TICKS);
        }

        break;

      case ACTIVATE:
        // poll for PG
        if (!bits) {
          // delay deactivation
          next_state(atx_psu, DEACTIVATE, atx_psu->options.timeout);

        } else if (power_good) {
          LOG_INFO("power-good");

          next_state(atx_psu, ACTIVE, ATX_PSU_ACTIVE_TICKS);

        } else if (timeout) {
          // re-ACTIVATE to test power_good
          next_state(atx_psu, ACTIVATE, ATX_PSU_ACTIVATE_TICKS);
        }

        break;

      case ACTIVE:
        // wait for clear bit, poll for PG
        if (!bits) {
          // delay deactivation
          next_state(atx_psu, DEACTIVATE, atx_psu->options.timeout);

        } else if (!power_good) {
          LOG_WARN("power-bad");

          // re-ACTIVATE to test power_good
          next_state(atx_psu, ACTIVATE, ATX_PSU_ACTIVATE_TICKS);

        } else if (timeout) {
          next_state(atx_psu, ACTIVE, ATX_PSU_ACTIVE_TICKS);
        }

        break;

      case DEACTIVATE:
        // wait for any bit or deactivate on timeout
        if (bits && power_good) {
          // remain active
          next_state(atx_psu, ACTIVE, ATX_PSU_ACTIVE_TICKS);
        } else if (bits) {
          // re-ACTIVATE to test power_good
          next_state(atx_psu, ACTIVATE, ATX_PSU_ACTIVATE_TICKS);
        } else if (timeout) {
          LOG_INFO("power-off");
          clear_power_enable(atx_psu);

          next_state(atx_psu, INACTIVE, ATX_PSU_INACTIVE_TICKS);
        } else {
          // remain in DEACTIVATE state
        }

        break;

      default:
        LOG_FATAL("state=%d", atx_psu->state);
    }
  }
}

void atx_psu_power_enable(struct atx_psu *atx_psu, enum atx_psu_bit bit)
{
  LOG_DEBUG("bit=%08x", ATX_PSU_BIT(bit));

  xEventGroupSetBits(atx_psu->bits, ATX_PSU_BIT(bit));
  xEventGroupSetBits(atx_psu->events, ATX_PSU_EVENTS_SET_BIT);
}

int atx_psu_power_good(struct atx_psu *atx_psu, enum atx_psu_bit bit, TickType_t timeout)
{
  EventBits_t bits;

  xEventGroupSetBits(atx_psu->bits, ATX_PSU_BIT(bit));
  xEventGroupSetBits(atx_psu->events, ATX_PSU_EVENTS_SET_BIT);

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
  LOG_DEBUG("bit=%08x", ATX_PSU_BIT(bit));

  xEventGroupClearBits(atx_psu->bits, ATX_PSU_BIT(bit));
  xEventGroupSetBits(atx_psu->events, ATX_PSU_EVENTS_CLEAR_BIT);
}
