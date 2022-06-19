#include <user_leds.h>
#include "user_leds.h"

#include <logging.h>

#include <esp_err.h>

static int user_leds_init(struct user_leds *leds, const struct user_leds_options options[])
{
  int err;

  for (unsigned i = 0; i < leds->count; i++) {
    if ((err = user_led_init(&leds->leds[i], i, options[i]))) {
      LOG_ERROR("user_led_init[%u]", i);
      return err;
    }
  }

  if (!(leds->event_group = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  return 0;
}

int user_leds_new(struct user_leds **ledsp, size_t count, const struct user_leds_options options[])
{
  struct user_leds *leds;
  int err;

  if (!(leds = calloc(1, sizeof(*leds)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  leds->count = count;

  if (!(leds->leds = calloc(count, sizeof(*leds->leds)))) {
    LOG_ERROR("calloc");
    err = -1;
    goto error;
  }

  if ((err = user_leds_init(leds, options))) {
    LOG_ERROR("user_leds_init");
    goto error;
  }

  *ledsp = leds;

  return 0;

error:
  free(leds->leds);
  free(leds);

  return err;
}

TickType_t user_leds_tick(struct user_leds *leds)
{
  TickType_t this_tick = xTaskGetTickCount();
  TickType_t next_tick = portMAX_DELAY;

  for (unsigned i = 0; i < leds->count; i++) {
    struct user_led *led = &leds->leds[i];

    if (led->input_tick <= this_tick) {
      TickType_t period = user_led_input_tick(led);
      TickType_t led_tick = user_led_input_schedule(led, period);

      LOG_DEBUG("[%u] mode=%d input state=%d tick=%u -> period=%u tick=%u", i, led->options.mode, led->input_state, led->input_state_tick, period, led_tick);
    }

    if (led->output_tick <= this_tick) {
      TickType_t period = user_led_output_tick(led);
      TickType_t led_tick = user_led_output_schedule(led, period);

      LOG_DEBUG("[%u] mode=%d output state=%d index=%u -> period=%u tick=%u", i, led->options.mode, led->output_state, led->output_state_index, period, led_tick);
    }

    if (led->input_tick < next_tick) {
      next_tick = led->input_tick;
    }
    if (led->output_tick < next_tick) {
      next_tick = led->output_tick;
    }
  }

  return next_tick;
}

TickType_t user_leds_schedule(struct user_leds *leds, TickType_t tick)
{
  TickType_t now = xTaskGetTickCount();

  if (tick == portMAX_DELAY) {
    LOG_DEBUG("now=%u tick=%u -> block", now, tick);

    // indefinite period
    return portMAX_DELAY;

  } else if (tick > now) {
    LOG_DEBUG("now=%u tick=%u -> wait %u", now, tick, tick - now);

    // future tick
    return tick - now;

  } else {
    LOG_DEBUG("now=%u tick=%u -> immediate", now, tick);

    // missed tick, catchup
    return 0;
  }
}

void user_leds_main(void *arg)
{
  struct user_leds *leds = arg;

  for (;;) {
    TickType_t tick = user_leds_tick(leds);
    TickType_t delay = user_leds_schedule(leds, tick);
    unsigned bits;

    if (!(bits = xEventGroupWaitBits(leds->event_group, USER_LEDS_EVENT_BITS, pdTRUE, pdFALSE, delay))) {
      // timeout, just tick() next frame
      continue;
    }

    LOG_DEBUG("bits=%08x", bits);

    for (unsigned i = 0; i < leds->count; i++) {
      if (bits & USER_LEDS_EVENT_BIT(i)) {
        user_led_update(&leds->leds[i]);
      }
    }
  }
}

int user_leds_get(struct user_leds *leds, unsigned index)
{
  if (index > leds->count) {
    LOG_ERROR("invalid index=%u for count=%u", index, leds->count);
    return -1;
  }

  struct user_led *led = &leds->leds[index];

  if (!(led->options.mode & USER_LEDS_MODE_INPUT_BIT)) {
    LOG_ERROR("[%u] not configured for input mode", index);
    return -1;
  }

  LOG_DEBUG("[%u] gpio=%d mode=%04x", index, led->options.gpio, led->options.mode);

  if (led->input_state_tick) {
    return 1;
  } else {
    return 0;
  }
}

int user_leds_set(struct user_leds *leds, unsigned index, enum user_leds_state state)
{
  if (index > leds->count) {
    LOG_ERROR("invalid index=%u for count=%u", index, leds->count);
    return -1;
  }

  struct user_led *led = &leds->leds[index];

  if (!(led->options.mode & USER_LEDS_MODE_OUTPUT_BIT)) {
    LOG_ERROR("[%u] not configured for output mode", index);
    return -1;
  }

  LOG_DEBUG("[%u] gpio=%d mode=%04x state=%d", index, led->options.gpio, led->options.mode, state);

  if (xQueueOverwrite(led->queue, &state) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    return -1;
  }

  xEventGroupSetBits(leds->event_group, USER_LEDS_EVENT_BIT(index));

  return 0;
}
