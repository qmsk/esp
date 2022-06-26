#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP8266
  // less than 32 GPIOs
  typedef int gpio_pin_t;
  typedef uint32_t gpio_pins_t;

  #define GPIO_PINS_FMT "%08x"
  #define GPIO_PINS_ARGS(x) x

  // TODO
  #define GPIO_I2C_ENABLED 0
#elif CONFIG_IDF_TARGET_ESP32
  // more than 32 GPIOs
  typedef int gpio_pin_t;
  typedef uint64_t gpio_pins_t;

  // newlib-nano printf does not support 64-bit formatting
  #define GPIO_PINS_FMT "%02x%08x"
  #define GPIO_PINS_ARGS(x) (uint32_t)(x >> 32), (uint32_t)(x & 0xffffffff)

  #define GPIO_I2C_ENABLED 1
#endif

enum gpio_type {
  GPIO_TYPE_HOST,
#if GPIO_I2C_ENABLED
  GPIO_TYPE_I2C_PCA9534,
  GPIO_TYPE_I2C_PCA9554,  /* With weak internal pull-up on inputs */
#endif
};

#if CONFIG_IDF_TARGET_ESP8266
  #define GPIO_HOST_PIN_COUNT 17
  #define GPIO_HOST_PIN(x) ((gpio_pin_t)(x))

  #define GPIO_HOST_PINS_NONE 0
  #define GPIO_HOST_PINS_ALL   0x1ffff
  #define GPIO_HOST_PINS_HOST  0x0ffff
  #define GPIO_HOST_PINS_RTC   0x10000
  #define GPIO_HOST_PINS(x) (((gpio_pins_t)(1)) << x)

  #define GPIO_HOST_PINS_GPIO0  GPIO_HOST_PINS(0)
  #define GPIO_HOST_PINS_GPIO1  GPIO_HOST_PINS(1)
  #define GPIO_HOST_PINS_GPIO2  GPIO_HOST_PINS(2)
  #define GPIO_HOST_PINS_GPIO3  GPIO_HOST_PINS(3)
  #define GPIO_HOST_PINS_GPIO4  GPIO_HOST_PINS(4)
  #define GPIO_HOST_PINS_GPIO5  GPIO_HOST_PINS(5)
  #define GPIO_HOST_PINS_GPIO6  GPIO_HOST_PINS(6)
  #define GPIO_HOST_PINS_GPIO7  GPIO_HOST_PINS(7)
  #define GPIO_HOST_PINS_GPIO8  GPIO_HOST_PINS(8)
  #define GPIO_HOST_PINS_GPIO9  GPIO_HOST_PINS(9)
  #define GPIO_HOST_PINS_GPIO10 GPIO_HOST_PINS(10)
  #define GPIO_HOST_PINS_GPIO11 GPIO_HOST_PINS(11)
  #define GPIO_HOST_PINS_GPIO12 GPIO_HOST_PINS(12)
  #define GPIO_HOST_PINS_GPIO13 GPIO_HOST_PINS(13)
  #define GPIO_HOST_PINS_GPIO14 GPIO_HOST_PINS(14)
  #define GPIO_HOST_PINS_GPIO15 GPIO_HOST_PINS(15)
  #define GPIO_HOST_PINS_GPIO16 GPIO_HOST_PINS(16) // RTC

#elif CONFIG_IDF_TARGET_ESP32
  // GPIO34-39 are not supported (input-only)
  #define GPIO_HOST_PIN_COUNT 34
  #define GPIO_HOST_PIN(x) ((gpio_pin_t)(x))

  #define GPIO_HOST_PINS_NONE 0
  #define GPIO_HOST_PINS_ALL 0x00000003ffffffff
  #define GPIO_HOST_PINS(x) (((gpio_pins_t)(1)) << x)

  #define GPIO_HOST_PINS_GPIO0  GPIO_HOST_PINS(0)
  #define GPIO_HOST_PINS_GPIO1  GPIO_HOST_PINS(1)
  #define GPIO_HOST_PINS_GPIO2  GPIO_HOST_PINS(2)
  #define GPIO_HOST_PINS_GPIO3  GPIO_HOST_PINS(3)
  #define GPIO_HOST_PINS_GPIO4  GPIO_HOST_PINS(4)
  #define GPIO_HOST_PINS_GPIO5  GPIO_HOST_PINS(5)
  #define GPIO_HOST_PINS_GPIO6  GPIO_HOST_PINS(6)
  #define GPIO_HOST_PINS_GPIO7  GPIO_HOST_PINS(7)
  #define GPIO_HOST_PINS_GPIO8  GPIO_HOST_PINS(8)
  #define GPIO_HOST_PINS_GPIO9  GPIO_HOST_PINS(9)
  #define GPIO_HOST_PINS_GPIO10 GPIO_HOST_PINS(10)
  #define GPIO_HOST_PINS_GPIO11 GPIO_HOST_PINS(11)
  #define GPIO_HOST_PINS_GPIO12 GPIO_HOST_PINS(12)
  #define GPIO_HOST_PINS_GPIO13 GPIO_HOST_PINS(13)
  #define GPIO_HOST_PINS_GPIO14 GPIO_HOST_PINS(14)
  #define GPIO_HOST_PINS_GPIO15 GPIO_HOST_PINS(15)
  #define GPIO_HOST_PINS_GPIO16 GPIO_HOST_PINS(16)
  #define GPIO_HOST_PINS_GPIO17 GPIO_HOST_PINS(17)
  #define GPIO_HOST_PINS_GPIO18 GPIO_HOST_PINS(18)
  #define GPIO_HOST_PINS_GPIO19 GPIO_HOST_PINS(19)
  #define GPIO_HOST_PINS_GPIO20 GPIO_HOST_PINS(20)
  #define GPIO_HOST_PINS_GPIO21 GPIO_HOST_PINS(21)
  #define GPIO_HOST_PINS_GPIO22 GPIO_HOST_PINS(22)
  #define GPIO_HOST_PINS_GPIO23 GPIO_HOST_PINS(23)
  #define GPIO_HOST_PINS_GPIO24 GPIO_HOST_PINS(24)
  #define GPIO_HOST_PINS_GPIO25 GPIO_HOST_PINS(25)
  #define GPIO_HOST_PINS_GPIO26 GPIO_HOST_PINS(26)
  #define GPIO_HOST_PINS_GPIO27 GPIO_HOST_PINS(27)
  #define GPIO_HOST_PINS_GPIO28 GPIO_HOST_PINS(28)
  #define GPIO_HOST_PINS_GPIO29 GPIO_HOST_PINS(29)
  #define GPIO_HOST_PINS_GPIO30 GPIO_HOST_PINS(30)
  #define GPIO_HOST_PINS_GPIO31 GPIO_HOST_PINS(31)
  #define GPIO_HOST_PINS_GPIO32 GPIO_HOST_PINS(32)
  #define GPIO_HOST_PINS_GPIO33 GPIO_HOST_PINS(33)
#endif

#if GPIO_I2C_ENABLED
  #include <freertos/FreeRTOS.h>
  #include <hal/i2c_types.h>

  #define GPIO_I2C_ADDR_MAX 0xff

  #define GPIO_I2C_PCA9554_PIN_COUNT 8
  #define GPIO_I2C_PCA9554_PINS_MASK 0xff
  #define GPIO_I2C_PCA9554_PINS(x) (((gpio_pins_t)(1)) << x)

  #define GPIO_I2C_PCA9554_ADDR(addr) (GPIO_I2C_PCA9554_ADDR_BASE | ((addr) & GPIO_I2C_PCA9554_ADDR_MASK))
  #define GPIO_I2C_PCA9554_ADDR_BASE 0x20
  #define GPIO_I2C_PCA9554_ADDR_MASK 0x07
#endif

/* Return gpio_out_pins bitmask for numbered pin, or 0 if invalid */
gpio_pins_t gpio_host_pin(gpio_pin_t pin);

struct gpio_options {
  enum gpio_type type;

  union {
  #if GPIO_I2C_ENABLED
    struct gpio_i2c_options {
      i2c_port_t port;
      uint8_t addr;
      TickType_t timeout;
    } i2c;
  #endif
  };

  // bitmask of input pins to enable
  gpio_pins_t in_pins;

  // bitmask of outputs pins to enable
  gpio_pins_t out_pins;

  // invert output for pins
  gpio_pins_t inverted_pins;
};

/* Setup output pins */
int gpio_setup(const struct gpio_options *options);

/* Disable output pins, leaving inputs floating */
int gpio_in_setup(const struct gpio_options *options, gpio_pins_t pins);

/* Read all input pins */
int gpio_in_get(const struct gpio_options *options, gpio_pins_t *pins);

/* Enable output pins */
int gpio_out_setup(const struct gpio_options *options, gpio_pins_t pins);

/* Clear all output pins */
int gpio_out_clear(const struct gpio_options *options);

/* Set specific output pins, clear others */
int gpio_out_set(const struct gpio_options *options, gpio_pins_t pins);

/* Set all output pins */
int gpio_out_set_all(const struct gpio_options *options);
