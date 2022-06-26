#include <user_leds.h>
#include "user_leds.h"

#include <logging.h>

#include <esp_err.h>

static int user_leds_init(struct user_leds *leds, struct gpio_options *gpio_options, const struct user_leds_options options[])
{
  int err;

  if (!(leds->event_group = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  for (unsigned i = 0; i < leds->count; i++) {
    if ((err = user_led_init(&leds->leds[i], i, gpio_options, options[i]))) {
      LOG_ERROR("user_led_init[%u]", i);
      return err;
    }
  }

  return 0;
}

static int user_leds_init_gpio(struct user_leds *leds, struct gpio_options *gpio_options)
{
  int err;

  for (unsigned i = 0; i < leds->count; i++) {
    struct user_led *led = &leds->leds[i];

    LOG_INFO("[%u] gpio_pin=%2d -> in_pins=" GPIO_PINS_FMT " out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT, i,
      led->options.gpio_pin,
      GPIO_PINS_ARGS((led->options.mode & USER_LEDS_MODE_INPUT_BIT) ? GPIO_PINS(led->options.gpio_pin) : GPIO_PINS_NONE),
      GPIO_PINS_ARGS((led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) ? GPIO_PINS(led->options.gpio_pin) : GPIO_PINS_NONE),
      GPIO_PINS_ARGS((led->options.mode & USER_LEDS_MODE_INVERTED_BIT) ? GPIO_PINS(led->options.gpio_pin) : GPIO_PINS_NONE)
    );

    if (led->options.mode & USER_LEDS_MODE_INPUT_BIT) {
      gpio_options->in_pins |= GPIO_PINS(led->options.gpio_pin);
    }
    if (led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
      gpio_options->out_pins |= GPIO_PINS(led->options.gpio_pin);
    }
    if (led->options.mode & USER_LEDS_MODE_INVERTED_BIT) {
      gpio_options->inverted_pins |= GPIO_PINS(led->options.gpio_pin);
    }
  }

  LOG_INFO("gpio in_pins=" GPIO_PINS_FMT " out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
    GPIO_PINS_ARGS(gpio_options->in_pins),
    GPIO_PINS_ARGS(gpio_options->out_pins),
    GPIO_PINS_ARGS(gpio_options->inverted_pins)
  );

  // TODO: initial input-only state for mixed input/output pins
  if ((err = gpio_setup(gpio_options))) {
    LOG_ERROR("gpio_setup");
    return err;
  }

  leds->gpio_options = gpio_options;

  // read initial state
  if ((err = user_leds_input(leds))) {
    LOG_ERROR("user_leds_input");
    return err;
  }

  for (unsigned i = 0; i < leds->count; i++) {
    user_led_input_init(&leds->leds[i]);
  }

  return 0;
}

int user_leds_new(struct user_leds **ledsp, struct gpio_options *gpio_options, size_t count, const struct user_leds_options options[])
{
  struct user_leds *leds;
  int err;

  if (!(leds = calloc(1, sizeof(*leds)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  leds->count = count;

  if (!(leds->leds = calloc(count, sizeof(*leds->leds))) && count) {
    LOG_ERROR("calloc");
    err = -1;
    goto error;
  }

  if ((err = user_leds_init(leds, gpio_options, options))) {
    LOG_ERROR("user_leds_init");
    goto error;
  }

  if ((err = user_leds_init_gpio(leds, gpio_options))) {
    LOG_ERROR("user_leds_init_gpio");
    goto error;
  }

  *ledsp = leds;

  return 0;

error:
  free(leds->leds);
  free(leds);

  return err;
}

int user_leds_input(struct user_leds *leds)
{
  gpio_pins_t input_pins = 0, input_bits = 0;
  int err;

  for (unsigned i = 0; i < leds->count; i++) {
    struct user_led *led = &leds->leds[i];

    if (led->input_state) {
      input_pins |= GPIO_PINS(led->options.gpio_pin);
    }
  }

  if (input_pins) {
    if ((err = gpio_in_setup(leds->gpio_options, input_pins))) {
      LOG_ERROR("gpio_in_setup");
      return err;
    }

    if ((err = gpio_in_get(leds->gpio_options, &input_bits))) {
      LOG_ERROR("gpio_in_get");
      return err;
    }
  }

  LOG_DEBUG("gpio input_pins=" GPIO_PINS_FMT " input_bits=" GPIO_PINS_FMT,
    GPIO_PINS_ARGS(input_pins),
    GPIO_PINS_ARGS(input_bits)
  );

  for (unsigned i = 0; i < leds->count; i++) {
    struct user_led *led = &leds->leds[i];

    led->input_bit = input_bits & GPIO_PINS(led->options.gpio_pin);
  }

  return 0;
}

TickType_t user_leds_tick(struct user_leds *leds)
{
  TickType_t this_tick = xTaskGetTickCount();
  TickType_t next_tick = portMAX_DELAY;

  user_leds_input(leds);

  for (unsigned i = 0; i < leds->count; i++) {
    struct user_led *led = &leds->leds[i];

    if (led->input_tick <= this_tick) {
      TickType_t period = user_led_input_tick(led);
      TickType_t led_tick = user_led_input_schedule(led, period);

      LOG_DEBUG("[%u] mode=%d input bit=%d state=%d tick=%u -> period=%u tick=%u", i, led->options.mode, led->input_bit, led->input_state, led->input_state_tick, period, led_tick);
    }

    if (led->output_tick <= this_tick) {
      TickType_t period = user_led_output_tick(led);
      TickType_t led_tick = user_led_output_schedule(led, period);

      LOG_DEBUG("[%u] mode=%d output bit=%d state=%d index=%u -> period=%u tick=%u", i, led->options.mode, led->output_bit, led->output_state, led->output_state_index, period, led_tick);
    }

    if (led->input_tick < next_tick) {
      next_tick = led->input_tick;
    }
    if (led->output_tick < next_tick) {
      next_tick = led->output_tick;
    }
  }

  user_leds_output(leds);

  return next_tick;
}

int user_leds_output(struct user_leds *leds)
{
  gpio_pins_t output_pins = 0, output_bits = 0;
  int err;

  for (unsigned i = 0; i < leds->count; i++) {
    struct user_led *led = &leds->leds[i];

    if (led->output_state && !led->input_state) {
      output_pins |= GPIO_PINS(led->options.gpio_pin);
    }

    if (led->output_bit) {
      output_bits |= GPIO_PINS(led->options.gpio_pin);
    }
  }

  LOG_DEBUG("gpio output_pins=" GPIO_PINS_FMT " output_bits=" GPIO_PINS_FMT,
    GPIO_PINS_ARGS(output_pins),
    GPIO_PINS_ARGS(output_bits)
  );

  // update outputs
  if ((err = gpio_out_setup(leds->gpio_options, output_pins))) {
    LOG_ERROR("gpio_out_setup");
    return err;
  }

  if ((err = gpio_out_set(leds->gpio_options, output_bits))) {
    LOG_ERROR("gpio_out_set");
    return err;
  }

  return 0;
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

    LOG_DEBUG("update bits=%08x", bits);

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

  LOG_DEBUG("[%u] gpio_pin=%d mode=%04x", index, led->options.gpio_pin, led->options.mode);

  if (led->input_bit) {
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

  LOG_DEBUG("[%u] gpio_pin=%d mode=%04x state=%d", index, led->options.gpio_pin, led->options.mode, state);

  if (xQueueOverwrite(led->queue, &state) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    return -1;
  }

  xEventGroupSetBits(leds->event_group, USER_LEDS_EVENT_BIT(index));

  return 0;
}
