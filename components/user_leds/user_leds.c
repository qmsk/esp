#include <user_leds.h>
#include "user_leds.h"

#include <logging.h>

#include <esp_err.h>

int user_leds_init(struct user_leds *leds, const struct user_leds_options *options)
{
  int err;

  leds->count = options->count;

  for (unsigned i = 0; i < options->count; i++) {
    if ((err = user_led_init(&leds->leds[i], i, options->leds[i]))) {
      LOG_ERROR("user_led_init[%u]", i);
      return err;
    }
  }

  if (!(leds->mutex = xSemaphoreCreateMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
    return -1;
  }

  if (!(leds->event_group = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  return 0;
}

int user_leds_new(struct user_leds **ledsp, const struct user_leds_options *options)
{
  struct user_leds *leds;
  int err;

  if (!(leds = calloc(1, sizeof(*leds)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(leds->leds = calloc(options->count, sizeof(*leds->leds)))) {
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
  TickType_t next_tick = 0;

  for (unsigned i = 0; i < leds->count; i++) {
    struct user_led *led = &leds->leds[i];
    TickType_t led_tick = led->tick;

    if (led_tick && led_tick <= this_tick) {
      TickType_t period = user_led_tick(led);
      led_tick = user_led_schedule(led, period);

      LOG_DEBUG("[%u] mode=%d state=%d -> period=%u tick=%u", i, led->options.mode, led->state, period, led_tick);
    }

    if (!next_tick || led_tick < next_tick) {
      next_tick = led_tick;
    }
  }

  return next_tick;
}

TickType_t user_leds_schedule(struct user_leds *leds, TickType_t tick)
{
  TickType_t now = xTaskGetTickCount();

  if (tick == 0) {
    // indefinite period
    return portMAX_DELAY;

  } else if (tick > now) {
    // future tick
    return now - now;

  } else {
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

    LOG_DEBUG("tick=%u delay=%u", tick, delay);

    if (!(bits = xEventGroupWaitBits(leds->event_group, USER_LEDS_EVENT_BITS, pdTRUE, pdFALSE, delay))) {
      // timeout, just tick() next frame
      continue;
    }

    for (unsigned i = 0; i < leds->count; i++) {
      if (bits & USER_LEDS_EVENT_BIT(i)) {
        user_led_update(&leds->leds[i]);
      }
    }
  }
}

int user_leds_set(struct user_leds *leds, unsigned index, enum user_leds_state state, TickType_t timeout)
{
  struct user_led *led = &leds->leds[index];
  struct user_leds_event event = { state };
  int err = 0;

  if (!xSemaphoreTake(leds->mutex, timeout)) {
    LOG_DEBUG("xSemaphoreTake: timeout");
    return 1;
  }

  LOG_DEBUG("[%u] gpio=%d state=%d", index, led->options.gpio, state);

  led->set_state = state;

  if (led->set_override) {
    // skip set op
    LOG_DEBUG("override");

  } else if (xQueueOverwrite(led->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    err = -1;
    goto error;
  }

error:
  if (!xSemaphoreGive(leds->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int user_leds_override(struct user_leds *leds, unsigned index, enum user_leds_state state)
{
  struct user_led *led = &leds->leds[index];
  struct user_leds_event event = { state };
  int err = 0;

  if (!xSemaphoreTake(leds->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  LOG_DEBUG("[%u] gpio=%d state=%d", index, led->options.gpio, state);

  led->set_override = true;

  if (xQueueOverwrite(led->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    err = -1;
    goto error;
  }

error:
  if (!xSemaphoreGive(leds->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int user_leds_revert(struct user_leds *leds, unsigned index)
{
  struct user_led *led = &leds->leds[index];
  struct user_leds_event event = { 0 };
  int err = 0;

  if (!xSemaphoreTake(leds->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  // use last mode set
  event.state = led->set_state;

  LOG_DEBUG("[%u] gpio=%d state=%d", index, led->options.gpio, event.state);

  led->set_override = false;

  if (xQueueOverwrite(led->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    err = -1;
    goto error;
  }

error:
  if (!xSemaphoreGive(leds->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int user_leds_read(struct user_leds *leds, unsigned index)
{
  struct user_led *led = &leds->leds[index];
  struct user_leds_event event = { USER_LEDS_READ };
  uint32_t notify_value;
  int ret = 0;

  if (!xSemaphoreTake(leds->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  // request notify
  led->read_task = xTaskGetCurrentTaskHandle();

  LOG_DEBUG("[%u] gpio=%d state=%d read_task=%p", index, led->options.gpio, event.state, led->read_task);

  // set input mode and read
  if (xQueueOverwrite(led->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    ret = -1;
    goto error;
  }

  // wait
  if (!xTaskNotifyWait(0, USER_LEDS_READ_NOTIFY_BIT, &notify_value, portMAX_DELAY)) {
    LOG_ERROR("xTaskNotifyWait");
    ret = -1;
    goto error;
  }

  LOG_DEBUG("[%u] gpio=%d notify_value=%04x", index, led->options.gpio, notify_value);

  if (notify_value & USER_LEDS_READ_NOTIFY_BIT) {
    ret = 1;
  }

error:
  if (!xSemaphoreGive(leds->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return ret;
}
