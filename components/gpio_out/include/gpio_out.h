#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP8266
  typedef int gpio_out_pin_t;
  typedef uint32_t gpio_out_pins_t;

  // GPIO16 is not supported (RTC)
  #define GPIO_OUT_PIN_COUNT 16
  #define GPIO_OUT_PIN(x) ((gpio_out_pin_t)(x))

  #define GPIO_OUT_PINS_FMT "%08x"
  #define GPIO_OUT_PINS_NONE 0
  #define GPIO_OUT_PINS_ALL 0xffff
  #define GPIO_OUT_PINS(x) ((gpio_out_pins_t)(1 << x))

  #define GPIO_OUT_PINS_GPIO0  GPIO_OUT_PINS(0)
  #define GPIO_OUT_PINS_GPIO1  GPIO_OUT_PINS(1)
  #define GPIO_OUT_PINS_GPIO2  GPIO_OUT_PINS(2)
  #define GPIO_OUT_PINS_GPIO3  GPIO_OUT_PINS(3)
  #define GPIO_OUT_PINS_GPIO4  GPIO_OUT_PINS(4)
  #define GPIO_OUT_PINS_GPIO5  GPIO_OUT_PINS(5)
  #define GPIO_OUT_PINS_GPIO6  GPIO_OUT_PINS(6)
  #define GPIO_OUT_PINS_GPIO7  GPIO_OUT_PINS(7)
  #define GPIO_OUT_PINS_GPIO8  GPIO_OUT_PINS(8)
  #define GPIO_OUT_PINS_GPIO9  GPIO_OUT_PINS(9)
  #define GPIO_OUT_PINS_GPIO10 GPIO_OUT_PINS(10)
  #define GPIO_OUT_PINS_GPIO11 GPIO_OUT_PINS(11)
  #define GPIO_OUT_PINS_GPIO12 GPIO_OUT_PINS(12)
  #define GPIO_OUT_PINS_GPIO13 GPIO_OUT_PINS(13)
  #define GPIO_OUT_PINS_GPIO14 GPIO_OUT_PINS(14)
  #define GPIO_OUT_PINS_GPIO15 GPIO_OUT_PINS(15)

#elif CONFIG_IDF_TARGET_ESP32
  #include <hal/gpio_types.h>

  typedef int gpio_out_pin_t;
  typedef uint64_t gpio_out_pins_t;

  // GPIO34-39 are not supported (input-only)
  #define GPIO_OUT_PIN_COUNT 34
  #define GPIO_OUT_PIN(x) ((gpio_out_pin_t)(x))

  #define GPIO_OUT_PINS_FMT "%010llx"
  #define GPIO_OUT_PINS_NONE 0
  #define GPIO_OUT_PINS_ALL 0x3ffffffff
  #define GPIO_OUT_PINS(x) ((gpio_out_pins_t)(1 << x))

  #define GPIO_OUT_PINS_GPIO0  GPIO_OUT_PINS(0)
  #define GPIO_OUT_PINS_GPIO1  GPIO_OUT_PINS(1)
  #define GPIO_OUT_PINS_GPIO2  GPIO_OUT_PINS(2)
  #define GPIO_OUT_PINS_GPIO3  GPIO_OUT_PINS(3)
  #define GPIO_OUT_PINS_GPIO4  GPIO_OUT_PINS(4)
  #define GPIO_OUT_PINS_GPIO5  GPIO_OUT_PINS(5)
  #define GPIO_OUT_PINS_GPIO6  GPIO_OUT_PINS(6)
  #define GPIO_OUT_PINS_GPIO7  GPIO_OUT_PINS(7)
  #define GPIO_OUT_PINS_GPIO8  GPIO_OUT_PINS(8)
  #define GPIO_OUT_PINS_GPIO9  GPIO_OUT_PINS(9)
  #define GPIO_OUT_PINS_GPIO10 GPIO_OUT_PINS(10)
  #define GPIO_OUT_PINS_GPIO11 GPIO_OUT_PINS(11)
  #define GPIO_OUT_PINS_GPIO12 GPIO_OUT_PINS(12)
  #define GPIO_OUT_PINS_GPIO13 GPIO_OUT_PINS(13)
  #define GPIO_OUT_PINS_GPIO14 GPIO_OUT_PINS(14)
  #define GPIO_OUT_PINS_GPIO15 GPIO_OUT_PINS(15)
  #define GPIO_OUT_PINS_GPIO16 GPIO_OUT_PINS(16)
  #define GPIO_OUT_PINS_GPIO17 GPIO_OUT_PINS(17)
  #define GPIO_OUT_PINS_GPIO18 GPIO_OUT_PINS(18)
  #define GPIO_OUT_PINS_GPIO19 GPIO_OUT_PINS(19)
  #define GPIO_OUT_PINS_GPIO20 GPIO_OUT_PINS(20)
  #define GPIO_OUT_PINS_GPIO21 GPIO_OUT_PINS(21)
  #define GPIO_OUT_PINS_GPIO22 GPIO_OUT_PINS(22)
  #define GPIO_OUT_PINS_GPIO23 GPIO_OUT_PINS(23)
  #define GPIO_OUT_PINS_GPIO24 GPIO_OUT_PINS(24)
  #define GPIO_OUT_PINS_GPIO25 GPIO_OUT_PINS(25)
  #define GPIO_OUT_PINS_GPIO26 GPIO_OUT_PINS(26)
  #define GPIO_OUT_PINS_GPIO27 GPIO_OUT_PINS(27)
  #define GPIO_OUT_PINS_GPIO28 GPIO_OUT_PINS(28)
  #define GPIO_OUT_PINS_GPIO29 GPIO_OUT_PINS(29)
  #define GPIO_OUT_PINS_GPIO30 GPIO_OUT_PINS(30)
  #define GPIO_OUT_PINS_GPIO31 GPIO_OUT_PINS(31)
  #define GPIO_OUT_PINS_GPIO32 GPIO_OUT_PINS(32)
  #define GPIO_OUT_PINS_GPIO33 GPIO_OUT_PINS(33)
#endif

/* Return gpio_out_pins bitmask for numbered pin, or 0 if invalid */
gpio_out_pins_t gpio_out_pin(gpio_out_pin_t pin);

struct gpio_out {
  // bitmask of pins to enable
  gpio_out_pins_t pins;

  // invert output for pins
  gpio_out_pins_t inverted;
};

/* Setup output pins */
int gpio_out_setup(struct gpio_out *out);

/* Clear all output pins */
void gpio_out_clear(struct gpio_out *out);

/* Set specific output pins, clear others */
void gpio_out_set(struct gpio_out *out, gpio_out_pins_t pins);

/* Set all output pins */
void gpio_out_all(struct gpio_out *out);
