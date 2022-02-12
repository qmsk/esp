#pragma once

#include <stdbool.h>

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP8266
  // GPIO16 is not supported
  #define GPIO_OUT_PIN_COUNT 16

  enum gpio_out_pins {
    GPIO_OUT_PIN_GPIO0    = 1 << 0,
    GPIO_OUT_PIN_GPIO1    = 1 << 1,
    GPIO_OUT_PIN_GPIO2    = 1 << 2,
    GPIO_OUT_PIN_GPIO3    = 1 << 3,
    GPIO_OUT_PIN_GPIO4    = 1 << 4,
    GPIO_OUT_PIN_GPIO5    = 1 << 5,
    GPIO_OUT_PIN_GPIO6    = 1 << 6,
    GPIO_OUT_PIN_GPIO7    = 1 << 7,
    GPIO_OUT_PIN_GPIO8    = 1 << 8,
    GPIO_OUT_PIN_GPIO9    = 1 << 9,
    GPIO_OUT_PIN_GPIO10   = 1 << 10,
    GPIO_OUT_PIN_GPIO11   = 1 << 11,
    GPIO_OUT_PIN_GPIO12   = 1 << 12,
    GPIO_OUT_PIN_GPIO13   = 1 << 13,
    GPIO_OUT_PIN_GPIO14   = 1 << 14,
    GPIO_OUT_PIN_GPIO15   = 1 << 15,
  };

#elif CONFIG_IDF_TARGET_ESP32
  #include <hal/gpio_types.h>

  // GPIO32-33 is not supported
  #define GPIO_OUT_PIN_COUNT 32

  enum gpio_out_pins {
    GPIO_OUT_PIN_GPIO0    = 1 << 0,
    GPIO_OUT_PIN_GPIO1    = 1 << 1,
    GPIO_OUT_PIN_GPIO2    = 1 << 2,
    GPIO_OUT_PIN_GPIO3    = 1 << 3,
    GPIO_OUT_PIN_GPIO4    = 1 << 4,
    GPIO_OUT_PIN_GPIO5    = 1 << 5,
    GPIO_OUT_PIN_GPIO6    = 1 << 6,
    GPIO_OUT_PIN_GPIO7    = 1 << 7,
    GPIO_OUT_PIN_GPIO8    = 1 << 8,
    GPIO_OUT_PIN_GPIO9    = 1 << 9,
    GPIO_OUT_PIN_GPIO10   = 1 << 10,
    GPIO_OUT_PIN_GPIO11   = 1 << 11,
    GPIO_OUT_PIN_GPIO12   = 1 << 12,
    GPIO_OUT_PIN_GPIO13   = 1 << 13,
    GPIO_OUT_PIN_GPIO14   = 1 << 14,
    GPIO_OUT_PIN_GPIO15   = 1 << 15,
    GPIO_OUT_PIN_GPIO16   = 1 << 16,
    GPIO_OUT_PIN_GPIO17   = 1 << 17,
    GPIO_OUT_PIN_GPIO18   = 1 << 18,
    GPIO_OUT_PIN_GPIO19   = 1 << 19,
    GPIO_OUT_PIN_GPIO20   = 1 << 20,
    GPIO_OUT_PIN_GPIO21   = 1 << 21,
    GPIO_OUT_PIN_GPIO22   = 1 << 22,
    GPIO_OUT_PIN_GPIO23   = 1 << 23,
    GPIO_OUT_PIN_GPIO24   = 1 << 24,
    GPIO_OUT_PIN_GPIO25   = 1 << 25,
    GPIO_OUT_PIN_GPIO26   = 1 << 26,
    GPIO_OUT_PIN_GPIO27   = 1 << 27,
    GPIO_OUT_PIN_GPIO28   = 1 << 28,
    GPIO_OUT_PIN_GPIO29   = 1 << 29,
    GPIO_OUT_PIN_GPIO30   = 1 << 30,
    GPIO_OUT_PIN_GPIO31   = 1 << 31,
  };
#endif

/* Return gpio_out_pins bitmask for numbered pin, or 0 if invalid */
enum gpio_out_pins gpio_out_pin(int pin);

enum gpio_out_level {
  GPIO_OUT_LOW          = 0,
  GPIO_OUT_HIGH         = 1,
};

struct gpio_out {
  enum gpio_out_pins pins;
  enum gpio_out_level level;
};

int gpio_out_init(struct gpio_out *out, enum gpio_out_pins pins, enum gpio_out_level level);

/* Clear all output pins */
void gpio_out_clear(struct gpio_out *out);

/* Set specific output pins, clear others */
void gpio_out_set(struct gpio_out *out, enum gpio_out_pins pins);

/* Set all output pins */
void gpio_out_all(struct gpio_out *out);
