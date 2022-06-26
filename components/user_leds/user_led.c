#include "user_leds.h"

#include <logging.h>

int user_led_init(struct user_led *led, unsigned index, struct gpio_options *gpio_options, struct user_leds_options options)
{
  led->index = index;
  led->options = options;

  if (options.mode & USER_LEDS_MODE_INPUT_BIT) {
    // initial read in input-only mode
    led->input_state = USER_LEDS_READ;
    led->input_tick = 0;
  } else {
    led->input_tick = portMAX_DELAY;
  }

  if (options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
    led->output_tick = 0;
  } else {
    led->output_tick = portMAX_DELAY;
  }

  if (options.mode & USER_LEDS_MODE_INTERRUPT_BIT) {
    if (!(led->input_interrupt = xSemaphoreCreateBinary())) {
      LOG_ERROR("xSemaphoreCreateBinary");
      return -1;
    }
  }

  // setup task
  if ((led->queue = xQueueCreate(1, sizeof(enum user_leds_state))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  return 0;
}

void user_led_input_init(struct user_led *led)
{
  if (led->options.mode & USER_LEDS_MODE_INPUT_BIT) {
    // read input and set initial state
    if (led->input_bit) {
      led->input_state_tick = xTaskGetTickCount();
    }
  }
}

static void user_led_input_event(struct user_led *led, struct user_leds_input input)
{
  LOG_DEBUG("gpio_pin=%d event index=%u type=%d press=%u hold=%u release=%u", led->options.gpio_pin,
    input.index,
    input.event,
    input.press,
    input.hold,
    input.release
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
    case USER_LEDS_READ_IDLE:
      if (led->output_state) {
        led->input_state = USER_LEDS_READ;

        LOG_DEBUG("[%u]: read wait", led->index);

        return USER_LEDS_READ_WAIT_TICKS;
      }

      /* fall-through */

    case USER_LEDS_READ:
      // read input and notify waiting task
      if (led->input_bit) {
        if (led->input_state_tick) {
          user_led_input_event(led, (struct user_leds_input) {
            .index  = led->index,
            .event  = USER_LEDS_INPUT_HOLD,
            .press  = led->input_state_tick,
            .hold   = xTaskGetTickCount() - led->input_state_tick,
          });
        } else {
          led->input_state_tick = xTaskGetTickCount();

          user_led_input_event(led, (struct user_leds_input) {
            .index  = led->index,
            .event  = USER_LEDS_INPUT_PRESS,
            .press  = led->input_state_tick,
          });
        }

      } else {
        if (led->input_state_tick) {
          TickType_t tick = xTaskGetTickCount();
          user_led_input_event(led, (struct user_leds_input) {
            .index    = led->index,
            .event    = USER_LEDS_INPUT_RELEASE,
            .press    = led->input_state_tick,
            .hold     = tick - led->input_state_tick,
            .release  = tick,
          });

          led->input_state_tick = 0;
        }
      }

      if (led->input_state_tick) {
        if (led->output_state) {
          led->input_state = USER_LEDS_READ_IDLE;
        } else {
          led->input_state = USER_LEDS_READ;
        }

        return USER_LEDS_READ_HOLD_TICKS;

      } else {
        led->input_state = USER_LEDS_READ_IDLE;

        return USER_LEDS_READ_IDLE_TICKS;
      }

    default:
      LOG_FATAL("invalid input_state=%d", led->input_state);
  }
}

TickType_t user_led_output_tick(struct user_led *led)
{
  switch(led->output_state) {
    case USER_LEDS_IDLE:
      led->output_bit = 0;
      led->output_state_index = 0;

      return portMAX_DELAY;

    case USER_LEDS_OFF:
      led->output_bit = 0;
      led->output_state_index = 0;

      return portMAX_DELAY;

    case USER_LEDS_ON:
      led->output_bit = 1;
      led->output_state_index = 1;

      return portMAX_DELAY;

    case USER_LEDS_SLOW:
      if (led->output_state_index) {
        led->output_bit = 0;
        led->output_state_index = 0;

        return USER_LEDS_SLOW_TICKS_OFF;
      } else {
        led->output_bit = 1;
        led->output_state_index = 1;

        return USER_LEDS_SLOW_TICKS_ON;
      }

    case USER_LEDS_FAST:
      if (led->output_state_index) {
        led->output_bit = 0;
        led->output_state_index = 0;
      } else {
        led->output_bit = 1;
        led->output_state_index = 1;
      }

      return USER_LEDS_FAST_TICKS;

    case USER_LEDS_FLASH:
      if (led->output_state_index) {
        led->output_bit = 0;
        led->output_state_index = 0;
        led->output_state = USER_LEDS_IDLE;

        return portMAX_DELAY;
      } else {
        led->output_bit = 1;
        led->output_state_index = 1;

        return USER_LEDS_FLASH_TICKS;
      }

    case USER_LEDS_PULSE:
      if (led->output_state_index) {
        led->output_bit = 0;
        led->output_state_index = 0;

        return USER_LEDS_PULSE_TICKS_OFF;
      } else {
        led->output_bit = 1;
        led->output_state_index = 1;

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

void user_led_update(struct user_led *led)
{
  enum user_leds_state state;

  if (led->options.mode & USER_LEDS_MODE_INTERRUPT_BIT) {
    if (xSemaphoreTake(led->input_interrupt, 0)) {
      LOG_DEBUG("[%u] isr fired", led->index);

      // reschedule immediate input read
      led->input_state = USER_LEDS_READ;
      led->input_tick = 0;
    }
  }

  if (led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
    if (!xQueueReceive(led->queue, &state, 0)) {
      LOG_DEBUG("[%u] queue empty", led->index);
    } else {
      LOG_DEBUG("[%u] queue state=%d", led->index, state);

      // reschedule immediate output write for state
      led->output_state = state;
      led->output_state_index = 0;
      led->output_tick = 0;
    }
  }
}
