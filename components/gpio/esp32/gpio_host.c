#include <gpio.h>
#include "../gpio.h"
#include "gpio_host.h"
#include "gpio_intr.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_rom_gpio.h>
#include <hal/gpio_types.h>
#include <hal/gpio_ll.h>
#include <hal/rtc_io_ll.h>
#include <soc/gpio_periph.h>
#include <soc/rtc_io_periph.h>

IRAM_ATTR void gpio_host_intr_handler (const struct gpio_options *options, gpio_pins_t pins)
{
  if (options->interrupt_func) {
    options->interrupt_func(pins & options->interrupt_pins, options->interrupt_arg);
  }
}

static void gpio_host_setup_rtc(gpio_pin_t gpio, bool pullup, bool pulldown)
{
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
  int rtc_ionum;

  if ((rtc_ionum = rtc_io_num_map[gpio]) < 0) {
    return;
  }

  if (pullup) {
    rtcio_ll_pullup_enable(rtc_ionum);
  } else {
    rtcio_ll_pullup_disable(rtc_ionum);
  }

  if (pulldown) {
    rtcio_ll_pulldown_enable(rtc_ionum);
  } else {
    rtcio_ll_pulldown_disable(rtc_ionum);
  }

  // configure RTC IOMUX as digital GPIO
  rtcio_ll_function_select(rtc_ionum, RTCIO_FUNC_DIGITAL);
#endif
}

static void gpio_host_set_pin(gpio_pin_t gpio, bool level)
{
  gpio_ll_set_level(&GPIO, gpio, level);
}

static void gpio_host_setup_pin(gpio_pin_t gpio, bool input, bool output, bool pullup, bool pulldown, gpio_int_type_t int_type)
{
  if (input) {
    gpio_ll_input_enable(&GPIO, gpio);
  } else {
    gpio_ll_input_disable(&GPIO, gpio);
  }

  if (output) {
    gpio_ll_output_enable(&GPIO, gpio);
  } else {
    gpio_ll_output_disable(&GPIO, gpio);
  }

  gpio_ll_od_disable(&GPIO, gpio);

  if (pullup) {
    gpio_ll_pullup_en(&GPIO, gpio);
  } else {
    gpio_ll_pullup_dis(&GPIO, gpio);
  }

  if (pulldown) {
    gpio_ll_pulldown_en(&GPIO, gpio);
  } else {
    gpio_ll_pulldown_dis(&GPIO, gpio);
  }

  gpio_ll_set_intr_type(&GPIO, gpio, int_type);

  // configure GPIO matrix as GPIO, to ensure that GPIO_FUNC is not driven by some other output signal
  gpio_host_setup_signal(gpio);

  // configure IOMUX as GPIO
  gpio_ll_iomux_func_sel(GPIO_PIN_MUX_REG[gpio], PIN_FUNC_GPIO);

  LOG_DEBUG("GPIO_PIN_MUX_REG[%d]@%#010x = %08x", gpio, GPIO_PIN_MUX_REG[gpio], REG_READ(GPIO_PIN_MUX_REG[gpio]));
  LOG_DEBUG("GPIO_FUNC%d_OUT_SEL_CFG_REG@%p = %08x", gpio, &GPIO.func_out_sel_cfg[gpio].val, GPIO.func_out_sel_cfg[gpio].val);
}

int gpio_host_setup_intr_pin(gpio_pin_t gpio, gpio_int_type_t int_type)
{
  int core = gpio_intr_core();

  gpio_host_setup_rtc(gpio, false, false);
  gpio_host_setup_pin(gpio, true, false, false, false, int_type);

  gpio_ll_intr_enable_on_core(&GPIO, core, gpio);

  return 0;
}

static void gpio_host_enable_interrupts(const struct gpio_options *options, gpio_pins_t pins)
{
  int core = gpio_intr_core();

  gpio_intr_clear(options->interrupt_pins & pins);

  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    if (options->interrupt_pins & GPIO_PINS(gpio)) {
      if (pins & GPIO_PINS(gpio)) {
        gpio_ll_intr_enable_on_core(&GPIO, core, gpio);
      }
    }
  }
}

static void gpio_host_disable_interrupts(const struct gpio_options *options, gpio_pins_t pins)
{
  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    if (options->interrupt_pins & GPIO_PINS(gpio)) {
      if (pins & GPIO_PINS(gpio)) {
        gpio_ll_intr_disable(&GPIO, gpio);
      }
    }
  }

  gpio_intr_clear(options->interrupt_pins & pins);
}

int gpio_host_setup(const struct gpio_options *options)
{
  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    bool input = options->in_pins & GPIO_PINS(gpio) & GPIO_HOST_PINS_INPUT;
    bool output = options->out_pins & GPIO_PINS(gpio) & GPIO_HOST_PINS_OUTPUT;
    bool inverted = options->inverted_pins & GPIO_PINS(gpio);
    bool interrupt = options->interrupt_pins & GPIO_PINS(gpio);

    if (input || output) {
      gpio_host_set_pin(gpio, inverted ? 1 : 0); // clear
      gpio_host_setup_pin(gpio, input, output, inverted, !inverted, interrupt ? GPIO_INTR_ANYEDGE : GPIO_INTR_DISABLE);
      gpio_host_setup_rtc(gpio, inverted, !inverted);
    }
    if (interrupt) {
      gpio_intr_setup_host_pin(gpio, options);
    }
  }

  return 0;
}

int gpio_host_setup_input(const struct gpio_options *options, gpio_pins_t pins)
{
  gpio_host_setup_pins(options->in_pins & options->out_pins, ~pins);
  gpio_host_enable_interrupts(options, pins);

  LOG_DEBUG("enable=%08x", GPIO.enable);

  return 0;
}

int gpio_host_get(const struct gpio_options *options, gpio_pins_t *pins)
{
  LOG_DEBUG("in=%08x", GPIO.in);

  *pins = gpio_host_get_pins(options->in_pins) ^ (options->inverted_pins & options->in_pins);

  return 0;
}

int gpio_host_setup_output(const struct gpio_options *options, gpio_pins_t pins)
{
  gpio_host_disable_interrupts(options, pins);
  gpio_host_setup_pins(options->out_pins, pins);

  LOG_DEBUG("enable=%08x", GPIO.enable);

  return 0;
}

int gpio_host_clear(const struct gpio_options *options)
{
  gpio_host_set_pins(options->out_pins, GPIO_HOST_PINS_NONE ^ options->inverted_pins);

  LOG_DEBUG("out=%08x", GPIO.out);

  return 0;
}

int gpio_host_set(const struct gpio_options *options, gpio_pins_t pins)
{
  gpio_host_set_pins(options->out_pins, pins ^ options->inverted_pins);

  LOG_DEBUG("out=%08x", GPIO.out);

  return 0;
}

int gpio_host_set_all(const struct gpio_options *options)
{
  gpio_host_set_pins(options->out_pins, GPIO_HOST_PINS_ALL ^ options->inverted_pins);

  LOG_DEBUG("out=%08x", GPIO.out);

  return 0;
}
