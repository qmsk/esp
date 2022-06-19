#include "user_leds.h"

#include <driver/gpio.h>

#include <logging.h>

static IRAM_ATTR void user_led_gpio_isr (void * arg)
{
  struct user_led *led = arg;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  LOG_ISR_DEBUG("[%u]", led->index);

  if (!xSemaphoreGiveFromISR(led->input_interrupt, &xHigherPriorityTaskWoken)) {
    LOG_ISR_DEBUG("xSemaphoreGiveFromISR");
  }

  if (!xEventGroupSetBitsFromISR(led->leds_event_group, USER_LEDS_EVENT_BIT(led->index), &xHigherPriorityTaskWoken)) {
    LOG_ISR_WARN("xEventGroupSetBitsFromISR");
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

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

  if (options.mode & USER_LEDS_MODE_INTERRUPT_BIT) {
    gpio_set_intr_type(options.gpio, GPIO_INTR_ANYEDGE);

    if ((err = gpio_isr_handler_add(options.gpio, user_led_gpio_isr, led))) {
      LOG_ERROR("gpio_isr_handler_add: %s", esp_err_to_name(err));
      return -1;
    }
  }

  return 0;
}

static inline void user_led_output_mode(struct user_led *led)
{
  LOG_DEBUG("gpio=%d", led->options.gpio);

  if (led->options.mode & USER_LEDS_MODE_INTERRUPT_BIT) {
    // do not trigger interrupts when driving pin in output mode
    gpio_intr_disable(led->options.gpio);
  }

  gpio_set_direction(led->options.gpio, (led->options.mode & USER_LEDS_MODE_INPUT_BIT) ? GPIO_MODE_INPUT_OUTPUT : GPIO_MODE_OUTPUT);
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

  if (led->options.mode & USER_LEDS_MODE_INTERRUPT_BIT) {
    gpio_intr_enable(led->options.gpio);
  }
}

static inline int user_led_input_read(struct user_led *led)
{
  int level = gpio_get_level(led->options.gpio);

  LOG_DEBUG("gpio=%d level=%d", led->options.gpio, level);

  return (led->options.mode & USER_LEDS_MODE_INVERTED_BIT) ? !level : level;
}

static int user_led_init_input(struct user_led *led)
{
  // read input and set initial state
  if (user_led_input_read(led)) {
    led->input_state_tick = xTaskGetTickCount();
  }

  if (led->options.mode & USER_LEDS_MODE_OUTPUT_BIT) {
    // revert back to output mode
    user_led_output_mode(led);
  }

  return 0;
}

int user_led_init(struct user_led *led, unsigned index, struct user_leds_options options, EventGroupHandle_t leds_event_group)
{
  int err;

  led->index = index;
  led->options = options;
  led->leds_event_group = leds_event_group;

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

  if (options.mode & USER_LEDS_MODE_INTERRUPT_BIT) {
    if (!(led->input_interrupt = xSemaphoreCreateBinary())) {
      LOG_ERROR("xSemaphoreCreateBinary");
      return -1;
    }
  }

  // enable GPIO output
  if ((err = user_led_init_gpio(led, index, options))) {
    LOG_ERROR("user_led_init_gpio[%u]", index);
    return err;
  }

  if (options.mode & USER_LEDS_MODE_INPUT_BIT) {
    if ((err = user_led_init_input(led))) {
      LOG_ERROR("user_led_init_input");
    }
  }

  // setup task
  if ((led->queue = xQueueCreate(1, sizeof(enum user_leds_state))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  return 0;
}

static void user_led_input_event(struct user_led *led, struct user_leds_input input)
{
  LOG_DEBUG("gpio=%d event index=%u type=%d press=%u hold=%u release=%u", led->options.gpio,
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
    case USER_LEDS_READ_INIT:
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
