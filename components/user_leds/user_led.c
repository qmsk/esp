#include "user_leds.h"

#include <driver/gpio.h>

#include <logging.h>

static int user_led_init_gpio(struct user_led *led, unsigned index, struct user_leds_options options)
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

  if (options.mode & USER_LEDS_MODE_INPUT_BIT) {
    // initialy input-only to read initial input state
    config.mode = GPIO_MODE_INPUT;
  } else if (options.mode & GPIO_MODE_DEF_OUTPUT) {
    config.mode = GPIO_MODE_OUTPUT;
  }

  if ((err = gpio_config(&config))) {
    LOG_ERROR("gpio_config: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int user_led_init(struct user_led *led, unsigned index, struct user_leds_options options)
{
  int err;

  led->index = index;
  led->options = options;

  if (options.mode & USER_LEDS_MODE_INPUT_BIT) {
    // initial read in input-only mode
    led->input_state = USER_LEDS_READ_INIT;
    led->input_tick = 0;
  } else {
    led->input_tick = portMAX_DELAY;
  }

  if (options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
    led->output_tick = 0;
  } else {
    led->output_tick = portMAX_DELAY;
  }

  // enable GPIO output
  if ((err = user_led_init_gpio(led, index, options))) {
    LOG_ERROR("user_led_init_gpio[%u]", index);
    return err;
  }

  // setup task
  if ((led->queue = xQueueCreate(1, sizeof(enum user_leds_state))) == NULL) {
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

static void user_led_input_event(struct user_led *led, struct user_leds_input input)
{
  LOG_DEBUG("gpio=%d event index=%u type=%d tick=%u ticks=%u", led->options.gpio,
    input.index,
    input.event,
    input.tick,
    input.ticks
  );

  if (led->options.input_queue) {
    xQueueOverwrite(led->options.input_queue, &input);
  }

  if (led->options.input_event_group) {
    xEventGroupSetBits(led->options.input_event_group, led->options.input_event_bits);
  }
}

TickType_t user_led_input_tick(struct user_led *led)
{
  // input states
  switch(led->input_state) {
    case USER_LEDS_READ_INIT:
      // read input and set initial state
      if (user_led_input_read(led)) {
        led->input_state_tick = xTaskGetTickCount();
      }

      if (led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
        // revert back to output mode
        user_led_output_mode(led);
      }

      led->input_state = USER_LEDS_READ_IDLE;

      if (led->input_state_tick) {
        return USER_LEDS_READ_HOLD_TICKS;
      } else {
        return USER_LEDS_READ_IDLE_TICKS;
      }

    case USER_LEDS_READ_IDLE:
      if (led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
        led->input_state = USER_LEDS_READ_WAIT;
      } else {
        led->input_state = USER_LEDS_READ;
      }

      return USER_LEDS_READ_IDLE_TICKS;

    case USER_LEDS_READ_WAIT:
    // let input pin settle before reading
      user_led_input_mode(led);

      led->input_state = USER_LEDS_READ;

      LOG_DEBUG("gpio=%d: read wait", led->options.gpio);

      return USER_LEDS_READ_WAIT_TICKS;

    case USER_LEDS_READ:
      // read input and notify waiting task
      if (user_led_input_read(led)) {
        if (led->input_state_tick) {
          user_led_input_event(led, (struct user_leds_input) {
            .index  = led->index,
            .event  = USER_LEDS_INPUT_HOLD,
            .tick   = led->input_state_tick,
            .ticks  = xTaskGetTickCount() - led->input_state_tick,
          });
        } else {
          led->input_state_tick = xTaskGetTickCount();

          user_led_input_event(led, (struct user_leds_input) {
            .index  = led->index,
            .event  = USER_LEDS_INPUT_PRESS,
            .tick   = led->input_state_tick,
            .ticks  = 0,
          });
        }

      } else {
        if (led->input_state_tick) {
          user_led_input_event(led, (struct user_leds_input) {
            .index  = led->index,
            .event  = USER_LEDS_INPUT_RELEASE,
            .tick   = led->input_state_tick,
            .ticks  = xTaskGetTickCount() - led->input_state_tick,
          });

          led->input_state_tick = 0;
        }
      }

      if (led->input_state_tick) {
        if (led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
          // revert back to output mode
          user_led_output_mode(led);

          led->input_state = USER_LEDS_READ_WAIT;
        } else {
          led->input_state = USER_LEDS_READ;
        }

        return USER_LEDS_READ_HOLD_TICKS;

      } else {
        led->input_state = USER_LEDS_READ_IDLE;

        if (led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
          // revert back to output mode
          user_led_output_mode(led);
        }

        return USER_LEDS_READ_IDLE_TICKS;
      }

    default:
      LOG_FATAL("invalid input_state=%d", led->input_state);
  }
}

TickType_t user_led_output_tick(struct user_led *led)
{
  switch(led->output_state) {
    case USER_LEDS_OFF:
      led->output_state_index = 0;

      user_led_output_off(led);

      return portMAX_DELAY;

    case USER_LEDS_ON:
      led->output_state_index = 1;

      user_led_output_on(led);

      return portMAX_DELAY;

    case USER_LEDS_SLOW:
      if (led->output_state_index) {
        led->output_state_index = 0;

        user_led_output_off(led);

        return USER_LEDS_SLOW_TICKS_OFF;
      } else {
        led->output_state_index = 1;

        user_led_output_on(led);

        return USER_LEDS_SLOW_TICKS_ON;
      }

    case USER_LEDS_FAST:
      if (led->output_state_index) {
        led->output_state_index = 0;

        user_led_output_off(led);
      } else {
        led->output_state_index = 1;

        user_led_output_on(led);
      }

      return USER_LEDS_FAST_TICKS;

    case USER_LEDS_FLASH:
      if (led->output_state_index) {

        user_led_output_off(led);

        return portMAX_DELAY;
      } else {
        user_led_output_on(led);

        led->output_state_index = 1;

        return USER_LEDS_FLASH_TICKS;
      }

    case USER_LEDS_PULSE:
      if (led->output_state_index) {
        led->output_state_index = 0;

        user_led_output_off(led);

        return USER_LEDS_PULSE_TICKS_OFF;
      } else {
        led->output_state_index = 1;

        user_led_output_on(led);

        return USER_LEDS_PULSE_TICKS_ON;
      }

    default:
      LOG_FATAL("invalid output_state=%d", led->output_state);
  }
}

// schedule next tick() period
TickType_t user_led_input_schedule(struct user_led *led, TickType_t period)
{
  TickType_t tick = xTaskGetTickCount();

  if (period == portMAX_DELAY) {
    // reset phase, indefinite period
    led->input_tick = portMAX_DELAY;

  } else if (led->input_tick == 0) {
    // start phase
    led->input_tick = tick + period;

  } else {
    // keep phase
    led->input_tick += period;
  }

  return led->input_tick;
}

// schedule next tick() period
TickType_t user_led_output_schedule(struct user_led *led, TickType_t period)
{
  TickType_t tick = xTaskGetTickCount();

  if (period == portMAX_DELAY) {
    // reset phase, indefinite period
    led->output_tick = portMAX_DELAY;

  } else if (led->output_tick == 0) {
    // start phase
    led->output_tick = tick + period;

  } else {
    // keep phase
    led->output_tick += period;
  }

  return led->output_tick;
}

static void user_led_output_state(struct user_led *led, enum user_leds_state state)
{
  LOG_DEBUG("gpio=%d: state=%d", led->options.gpio, state);

  switch(led->output_state = state) {
    case USER_LEDS_OFF:
      led->output_state_index = 0;

      break;

    case USER_LEDS_ON:
      led->output_state_index = 1;

      break;

    case USER_LEDS_SLOW:
    case USER_LEDS_FAST:
    case USER_LEDS_FLASH:
    case USER_LEDS_PULSE:
      led->output_state_index = 0;

      break;
  }

  // reschedule immediate tick
  led->output_tick = 0;
}

void user_led_update(struct user_led *led)
{
  enum user_leds_state state;

  if (!xQueueReceive(led->queue, &state, 0)) {
    LOG_WARN("spurious update, no state in queue");
    return;
  }

  user_led_output_state(led, state);
}
