#include "user_leds.h"

#include <driver/gpio.h>

#include <logging.h>

static int user_led_init_gpio(struct user_led *led, unsigned index, const struct user_led_options options)
{
  gpio_config_t config = {
      .pin_bit_mask = (1 << options.gpio),
      .mode         = (
          ((options.mode & USER_LEDS_MODE_INPUT_BIT) ? GPIO_MODE_DEF_INPUT : 0)
        | ((options.mode & USER_LEDS_MODE_OUTPUT_BIT) ? GPIO_MODE_DEF_OUTPUT : 0)
      ),
      .pull_up_en   = (options.mode & USER_LEDS_MODE_INVERTED_BIT) ? 1 : 0,
      .pull_down_en = (options.mode & USER_LEDS_MODE_INVERTED_BIT) ? 0 : 1,
  };
  esp_err_t err;

  LOG_INFO("[%u] gpio=%d input=%d output=%d inverted=%d", index,
    options.gpio,
    options.mode & USER_LEDS_MODE_INPUT_BIT,
    options.mode & USER_LEDS_MODE_OUTPUT_BIT,
    options.mode & USER_LEDS_MODE_INVERTED_BIT
  );

  if ((err = gpio_config(&config))) {
    LOG_ERROR("gpio_config: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int user_led_init(struct user_led *led, unsigned index, const struct user_led_options options)
{
  int err;

  led->options = options;

  // enable GPIO output
  if ((err = user_led_init_gpio(led, index, options))) {
    LOG_ERROR("user_led_init_gpio[%u]", index);
    return err;
  }

  // setup task
  if ((led->queue = xQueueCreate(1, sizeof(struct user_leds_event))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  return 0;
}

static inline void user_led_output_mode(struct user_led *led)
{
  LOG_DEBUG("gpio=%d", led->options.gpio);

  gpio_set_direction(led->options.gpio, GPIO_MODE_OUTPUT);
}

static inline void user_led_output_off(struct user_led *led)
{
  gpio_set_level(led->options.gpio, (led->options.mode & USER_LEDS_MODE_INVERTED_BIT) ? 1 : 0);
}

static inline void user_led_output_on(struct user_led *led)
{
  gpio_set_level(led->options.gpio, (led->options.mode & USER_LEDS_MODE_INVERTED_BIT) ? 0 : 1);
}

static inline void user_led_input_mode(struct user_led *led)
{
  LOG_DEBUG("gpio=%d", led->options.gpio);

  // disable output
  gpio_set_direction(led->options.gpio, GPIO_MODE_INPUT);
}

static inline int user_led_input_read(struct user_led *led)
{
  int level = gpio_get_level(led->options.gpio);

  LOG_DEBUG("gpio=%d level=%d", led->options.gpio, level);

  return (led->options.mode & USER_LEDS_MODE_INVERTED_BIT) ? !level : level;
}

static void user_led_read_notify(struct user_led *led, int level)
{
  LOG_DEBUG("gpio=%d read_task=%p level=%d", led->options.gpio, led->read_task, level);

  if (led->read_task) {
    xTaskNotify(led->read_task, level ? USER_LEDS_READ_NOTIFY_BIT : 0, eSetBits);

    led->read_task = false;
  }
}

TickType_t user_led_tick(struct user_led *led)
{
  // input states
  switch(led->state) {
    case USER_LEDS_READ_WAIT:
      // wait for input to settle
      led->state = USER_LEDS_READ;

      LOG_DEBUG("gpio=%d: input wait", led->options.gpio);

      return USER_LEDS_READ_WAIT_PERIOD / portTICK_RATE_MS;

    case USER_LEDS_READ:
      // read input and notify waiting task
      user_led_read_notify(led, user_led_input_read(led));

      if (led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
        // revert back to previous output mode
        led->state = led->output_state;

        user_led_output_mode(led);
      }

      break;

    default:
      break;
  }

  switch(led->state) {
    case USER_LEDS_OFF:
      led->state_index = 0;

      user_led_output_off(led);

      return 0;

    case USER_LEDS_ON:
      led->state_index = 1;

      user_led_output_on(led);

      return 0;

    case USER_LEDS_SLOW:
      if (led->state_index) {
        led->state_index = 0;

        user_led_output_off(led);

        return USER_LEDS_SLOW_PERIOD_OFF / portTICK_RATE_MS;
      } else {
        led->state_index = 1;

        user_led_output_on(led);

        return USER_LEDS_SLOW_PERIOD_ON / portTICK_RATE_MS;
      }

    case USER_LEDS_FAST:
      if (led->state_index) {
        led->state_index = 0;

        user_led_output_off(led);
      } else {
        led->state_index = 1;

        user_led_output_on(led);
      }

      return USER_LEDS_FAST_PERIOD / portTICK_RATE_MS;

    case USER_LEDS_FLASH:
      if (led->state_index) {

        user_led_output_off(led);

        return 0;
      } else {
        led->state_index = 1;

        return USER_LEDS_FLASH_PERIOD / portTICK_RATE_MS;
      }

    case USER_LEDS_PULSE:
      if (led->state_index) {
        led->state_index = 0;

        user_led_output_off(led);

        return USER_LEDS_PULSE_PERIOD_OFF / portTICK_RATE_MS;
      } else {
        led->state_index = 1;

        user_led_output_on(led);

        return USER_LEDS_PULSE_PERIOD_ON / portTICK_RATE_MS;
      }

    default:
      return 0;
  }
}

// schedule next tick() period
TickType_t user_led_schedule(struct user_led *led, TickType_t period)
{
  TickType_t tick = xTaskGetTickCount();

  if (period == 0) {
    // reset phase, indefinite period
    led->tick = 0;

  } else if (led->tick == 0) {
    // start phase
    led->tick = tick + period;

  } else {
    // keep phase
    led->tick += period;
  }

  return led->tick;
}

static void user_led_set_state(struct user_led *led, enum user_leds_state state)
{
  LOG_DEBUG("gpio=%d: state=%d", led->options.gpio, state);

  switch(led->state = state) {
    case USER_LEDS_OFF:
      led->output_state = state;
      led->state_index = 0;

      user_led_output_mode(led);
      user_led_output_off(led);

      break;

    case USER_LEDS_ON:
      led->output_state = state;
      led->state_index = 1;

      user_led_output_mode(led);
      user_led_output_on(led);

      break;

    case USER_LEDS_SLOW:
    case USER_LEDS_FAST:
    case USER_LEDS_FLASH:
    case USER_LEDS_PULSE:
      led->output_state = state;
      led->state_index = 0;

      user_led_output_mode(led);
      user_led_output_on(led);

      break;

    case USER_LEDS_READ:
      if (led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
        // let input settle before reading
        led->state = USER_LEDS_READ_WAIT;
      }

      /* fall-through */

    case USER_LEDS_READ_WAIT:
      user_led_output_off(led);
      user_led_input_mode(led);
  }
}

void user_led_update(struct user_led *led)
{
  struct user_leds_event event;

  if (!xQueueReceive(led->queue, &event, 0)) {
    LOG_WARN("spurious update, no event in queue");
    return;
  }

  user_led_set_state(led, event.state);
}
