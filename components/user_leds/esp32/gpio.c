#include "../user_leds.h"

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

int user_led_init_gpio(struct user_led *led, unsigned index, struct user_leds_options options)
{
  gpio_config_t config = {
      .pin_bit_mask = (1 << options.gpio_pin),
      .mode         = (
          ((options.mode & USER_LEDS_MODE_INPUT_BIT) ? GPIO_MODE_DEF_INPUT : 0)
        | ((options.mode & USER_LEDS_MODE_OUTPUT_BIT) ? GPIO_MODE_DEF_OUTPUT : 0)
      ),

      // XXX: pull-ups are only needed for inputs?
      .pull_up_en   = (options.mode & USER_LEDS_MODE_INVERTED_BIT) ? 1 : 0,
      .pull_down_en = (options.mode & USER_LEDS_MODE_INVERTED_BIT) ? 0 : 1,
  };
  esp_err_t err;

  LOG_INFO("[%u] gpio=%d input=%d output=%d inverted=%d", index,
    options.gpio_pin,
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
    gpio_set_intr_type(options.gpio_pin, GPIO_INTR_ANYEDGE);

    if ((err = gpio_isr_handler_add(options.gpio_pin, user_led_gpio_isr, led))) {
      LOG_ERROR("gpio_isr_handler_add: %s", esp_err_to_name(err));
      return -1;
    }
  }

  return 0;
}
