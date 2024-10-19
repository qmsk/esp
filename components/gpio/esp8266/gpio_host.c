#include <gpio.h>
#include "../gpio.h"
#include "gpio_host.h"

#include <logging.h>

#include <driver/gpio.h>
#include <esp_attr.h>
#include <esp_err.h>
#include <esp8266/gpio_struct.h>

#define GPIO_PIN_REG(x) ((volatile gpio_pin_reg_t *)(x))

static volatile gpio_pin_reg_t *rtc_pin_reg = GPIO_PIN_REG(PAD_XPD_DCDC_CONF);

static volatile gpio_pin_reg_t *gpio_pin_reg[16] = {
  [0]   = GPIO_PIN_REG(PERIPHS_IO_MUX_GPIO0_U),
  [1]   = GPIO_PIN_REG(PERIPHS_IO_MUX_U0TXD_U),
  [2]   = GPIO_PIN_REG(PERIPHS_IO_MUX_GPIO2_U),
  [3]   = GPIO_PIN_REG(PERIPHS_IO_MUX_U0RXD_U),
  [4]   = GPIO_PIN_REG(PERIPHS_IO_MUX_GPIO4_U),
  [5]   = GPIO_PIN_REG(PERIPHS_IO_MUX_GPIO5_U),
  [6]   = GPIO_PIN_REG(PERIPHS_IO_MUX_SD_CLK_U),
  [7]   = GPIO_PIN_REG(PERIPHS_IO_MUX_SD_DATA0_U),
  [8]   = GPIO_PIN_REG(PERIPHS_IO_MUX_SD_DATA1_U),
  [9]   = GPIO_PIN_REG(PERIPHS_IO_MUX_SD_DATA2_U),
  [10]  = GPIO_PIN_REG(PERIPHS_IO_MUX_SD_DATA3_U),
  [11]  = GPIO_PIN_REG(PERIPHS_IO_MUX_SD_CMD_U),
  [12]  = GPIO_PIN_REG(PERIPHS_IO_MUX_MTDI_U),
  [13]  = GPIO_PIN_REG(PERIPHS_IO_MUX_MTCK_U),
  [14]  = GPIO_PIN_REG(PERIPHS_IO_MUX_MTMS_U),
  [15]  = GPIO_PIN_REG(PERIPHS_IO_MUX_MTDO_U),
};

static const uint32_t gpio_mux_reg[16] = {
  [0]   = PERIPHS_IO_MUX_GPIO0_U,
  [1]   = PERIPHS_IO_MUX_U0TXD_U,
  [2]   = PERIPHS_IO_MUX_GPIO2_U,
  [3]   = PERIPHS_IO_MUX_U0RXD_U,
  [4]   = PERIPHS_IO_MUX_GPIO4_U,
  [5]   = PERIPHS_IO_MUX_GPIO5_U,
  [6]   = PERIPHS_IO_MUX_SD_CLK_U,
  [7]   = PERIPHS_IO_MUX_SD_DATA0_U,
  [8]   = PERIPHS_IO_MUX_SD_DATA1_U,
  [9]   = PERIPHS_IO_MUX_SD_DATA2_U,
  [10]  = PERIPHS_IO_MUX_SD_DATA3_U,
  [11]  = PERIPHS_IO_MUX_SD_CMD_U,
  [12]  = PERIPHS_IO_MUX_MTDI_U,
  [13]  = PERIPHS_IO_MUX_MTCK_U,
  [14]  = PERIPHS_IO_MUX_MTMS_U,
  [15]  = PERIPHS_IO_MUX_MTDO_U,
};

static const uint32_t gpio_mux_func[16] = {
  [0]   = FUNC_GPIO0,
  [1]   = FUNC_GPIO1,
  [2]   = FUNC_GPIO2,
  [3]   = FUNC_GPIO3,
  [4]   = FUNC_GPIO4,
  [5]   = FUNC_GPIO5,
  [6]   = FUNC_GPIO6,
  [7]   = FUNC_GPIO7,
  [8]   = FUNC_GPIO8,
  [9]   = FUNC_GPIO9,
  [10]  = FUNC_GPIO10,
  [11]  = FUNC_GPIO11,
  [12]  = FUNC_GPIO12,
  [13]  = FUNC_GPIO13,
  [14]  = FUNC_GPIO14,
  [15]  = FUNC_GPIO15,
};

IRAM_ATTR void gpio_host_intr_handler (const struct gpio_options *options, gpio_pins_t pins)
{
  if (options->interrupt_func) {
    options->interrupt_func(pins & options->interrupt_pins, options->interrupt_arg);
  }
}

static void gpio_host_setup_rtc(bool output, bool inverted)
{
  // input is not supported

  if (output) {
    SET_PERI_REG_MASK(RTC_GPIO_ENABLE, RTC_GPIO_BIT);
  } else {
    CLEAR_PERI_REG_MASK(RTC_GPIO_ENABLE, RTC_GPIO_BIT);
  }

  if (inverted) {
    rtc_pin_reg->rtc_pin.pulldown = false;
  } else {
    rtc_pin_reg->rtc_pin.pulldown = true;
  }

  gpio_rtc_setup_pin();
}

static void gpio_host_setup_pin(gpio_pin_t gpio, bool input, bool output, bool inverted, bool interrupt)
{
  if (inverted) {
    GPIO.out_w1ts = GPIO_PINS(gpio);
  } else {
    GPIO.out_w1tc = GPIO_PINS(gpio);
  }

  // input is always connected

  if (output) {
    GPIO.enable_w1ts = GPIO_PINS(gpio);
  } else {
    GPIO.enable_w1tc = GPIO_PINS(gpio);
  }

  // non-OD
  GPIO.pin[gpio].driver = 1;

  if (inverted) {
    gpio_pin_reg[gpio]->pullup = 1;
  } else {
    gpio_pin_reg[gpio]->pullup = 0;
  }

  if (interrupt) {
    GPIO.pin[gpio].int_type = GPIO_INTR_ANYEDGE;
  }

  IDEMPOTENT_PIN_FUNC_SELECT(gpio_mux_reg[gpio], gpio_mux_func[gpio]);
}

static void gpio_host_enable_interrupts(const struct gpio_options *options, gpio_pins_t pins)
{
  GPIO.status_w1tc = options->interrupt_pins & pins;

  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    if (options->interrupt_pins & GPIO_PINS(gpio)) {
      if (pins & GPIO_PINS(gpio)) {
        GPIO.pin[gpio].int_type = GPIO_INTR_ANYEDGE;
      }
    }
  }
}

static void gpio_host_disable_interrupts(const struct gpio_options *options, gpio_pins_t pins)
{
  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    if (options->interrupt_pins & GPIO_PINS(gpio)) {
      if (pins & GPIO_PINS(gpio)) {
        GPIO.pin[gpio].int_type = GPIO_INTR_DISABLE;
      }
    }
  }

  GPIO.status_w1tc = options->interrupt_pins & pins;
}

int gpio_host_setup(const struct gpio_options *options)
{
  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    bool input = options->in_pins & GPIO_PINS(gpio);
    bool output = options->out_pins & GPIO_PINS(gpio);
    bool inverted = options->inverted_pins & GPIO_PINS(gpio);
    bool interrupt = options->interrupt_pins & GPIO_PINS(gpio);

    if (input || output) {
      if (GPIO_HOST_PINS_RTC & GPIO_PINS(gpio)) {
        gpio_host_setup_rtc(output, inverted);
      } else {
        gpio_host_setup_pin(gpio, input, output, inverted, interrupt);
      }
    }
    if (interrupt) {
      if (GPIO_HOST_PINS_HOST & GPIO_PINS(gpio)) {
        gpio_intr_setup_host_pin(gpio, options);
      }
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
