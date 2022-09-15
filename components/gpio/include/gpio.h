#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP8266
  // less than 32 GPIOs
  typedef int gpio_pin_t;
  typedef uint32_t gpio_pins_t;

  #define GPIO_PIN_MAX 16

  #define GPIO_PINS_FMT "%08x"
  #define GPIO_PINS_ARGS(x) x

  // TODO
  #define GPIO_I2C_ENABLED 0
#elif CONFIG_IDF_TARGET_ESP32
  // more than 32 GPIOs
  typedef int gpio_pin_t;
  typedef uint64_t gpio_pins_t;

  #define GPIO_PIN_MAX 39

  // newlib-nano printf does not support 64-bit formatting
  #define GPIO_PINS_FMT "%02x%08x"
  #define GPIO_PINS_ARGS(x) (uint32_t)((x) >> 32), (uint32_t)((x) & 0xffffffff)

  #define GPIO_I2C_ENABLED 1
#endif

// gpio_pin_t -> gpio_pins_t bitmask
#define GPIO_PINS(x) (((gpio_pins_t)(1)) << x)
#define GPIO_PINS_NONE ((gpio_pins_t)(0))

enum gpio_type {
  GPIO_TYPE_HOST,
#if GPIO_I2C_ENABLED
  GPIO_TYPE_I2C,
#endif
};

#if CONFIG_IDF_TARGET_ESP8266
  #define GPIO_HOST_PIN_COUNT 17
  #define GPIO_HOST_PIN(x) ((gpio_pin_t)(x))

  #define GPIO_HOST_PINS_NONE 0
  #define GPIO_HOST_PINS_ALL   0x1ffff
  #define GPIO_HOST_PINS_HOST  0x0ffff
  #define GPIO_HOST_PINS_RTC   0x10000

  #define GPIO_HOST_PINS_GPIO0  GPIO_PINS(0)
  #define GPIO_HOST_PINS_GPIO1  GPIO_PINS(1)
  #define GPIO_HOST_PINS_GPIO2  GPIO_PINS(2)
  #define GPIO_HOST_PINS_GPIO3  GPIO_PINS(3)
  #define GPIO_HOST_PINS_GPIO4  GPIO_PINS(4)
  #define GPIO_HOST_PINS_GPIO5  GPIO_PINS(5)
  #define GPIO_HOST_PINS_GPIO6  GPIO_PINS(6)
  #define GPIO_HOST_PINS_GPIO7  GPIO_PINS(7)
  #define GPIO_HOST_PINS_GPIO8  GPIO_PINS(8)
  #define GPIO_HOST_PINS_GPIO9  GPIO_PINS(9)
  #define GPIO_HOST_PINS_GPIO10 GPIO_PINS(10)
  #define GPIO_HOST_PINS_GPIO11 GPIO_PINS(11)
  #define GPIO_HOST_PINS_GPIO12 GPIO_PINS(12)
  #define GPIO_HOST_PINS_GPIO13 GPIO_PINS(13)
  #define GPIO_HOST_PINS_GPIO14 GPIO_PINS(14)
  #define GPIO_HOST_PINS_GPIO15 GPIO_PINS(15)
  #define GPIO_HOST_PINS_GPIO16 GPIO_PINS(16) // RTC

#elif CONFIG_IDF_TARGET_ESP32
  // GPIO34-39 are not supported (input-only)
  #define GPIO_HOST_PIN_COUNT 40
  #define GPIO_HOST_PIN(x) ((gpio_pin_t)(x))

  #define GPIO_HOST_PINS_NONE 0
  #define GPIO_HOST_PINS_ALL    0x00000003ffffffff
  #define GPIO_HOST_PINS_INPUT  0x0000003fffffffff
  #define GPIO_HOST_PINS_OUTPUT 0x00000003ffffffff

  #define GPIO_HOST_PINS_GPIO0  GPIO_PINS(0)
  #define GPIO_HOST_PINS_GPIO1  GPIO_PINS(1)
  #define GPIO_HOST_PINS_GPIO2  GPIO_PINS(2)
  #define GPIO_HOST_PINS_GPIO3  GPIO_PINS(3)
  #define GPIO_HOST_PINS_GPIO4  GPIO_PINS(4)
  #define GPIO_HOST_PINS_GPIO5  GPIO_PINS(5)
  #define GPIO_HOST_PINS_GPIO6  GPIO_PINS(6)
  #define GPIO_HOST_PINS_GPIO7  GPIO_PINS(7)
  #define GPIO_HOST_PINS_GPIO8  GPIO_PINS(8)
  #define GPIO_HOST_PINS_GPIO9  GPIO_PINS(9)
  #define GPIO_HOST_PINS_GPIO10 GPIO_PINS(10)
  #define GPIO_HOST_PINS_GPIO11 GPIO_PINS(11)
  #define GPIO_HOST_PINS_GPIO12 GPIO_PINS(12)
  #define GPIO_HOST_PINS_GPIO13 GPIO_PINS(13)
  #define GPIO_HOST_PINS_GPIO14 GPIO_PINS(14)
  #define GPIO_HOST_PINS_GPIO15 GPIO_PINS(15)
  #define GPIO_HOST_PINS_GPIO16 GPIO_PINS(16)
  #define GPIO_HOST_PINS_GPIO17 GPIO_PINS(17)
  #define GPIO_HOST_PINS_GPIO18 GPIO_PINS(18)
  #define GPIO_HOST_PINS_GPIO19 GPIO_PINS(19)
  #define GPIO_HOST_PINS_GPIO20 GPIO_PINS(20) // XXX
  #define GPIO_HOST_PINS_GPIO21 GPIO_PINS(21)
  #define GPIO_HOST_PINS_GPIO22 GPIO_PINS(22)
  #define GPIO_HOST_PINS_GPIO23 GPIO_PINS(23)
  #define GPIO_HOST_PINS_GPIO24 GPIO_PINS(24) // XXX
  #define GPIO_HOST_PINS_GPIO25 GPIO_PINS(25)
  #define GPIO_HOST_PINS_GPIO26 GPIO_PINS(26)
  #define GPIO_HOST_PINS_GPIO27 GPIO_PINS(27)
  #define GPIO_HOST_PINS_GPIO28 GPIO_PINS(28) // XXX
  #define GPIO_HOST_PINS_GPIO29 GPIO_PINS(29) // XXX
  #define GPIO_HOST_PINS_GPIO30 GPIO_PINS(30) // XXX
  #define GPIO_HOST_PINS_GPIO31 GPIO_PINS(31) // XXX
  #define GPIO_HOST_PINS_GPIO32 GPIO_PINS(32)
  #define GPIO_HOST_PINS_GPIO33 GPIO_PINS(33)
  #define GPIO_HOST_PINS_GPIO34 GPIO_PINS(34) // input-only
  #define GPIO_HOST_PINS_GPIO35 GPIO_PINS(35) // input-only
  #define GPIO_HOST_PINS_GPIO36 GPIO_PINS(36) // XXX
  #define GPIO_HOST_PINS_GPIO37 GPIO_PINS(37) // XXX
  #define GPIO_HOST_PINS_GPIO38 GPIO_PINS(38) // input-only
  #define GPIO_HOST_PINS_GPIO39 GPIO_PINS(39) // input-only
#endif

#if GPIO_I2C_ENABLED
  #include <freertos/FreeRTOS.h>
  #include <hal/i2c_types.h>

  #define GPIO_I2C_ADDR_MAX 0xff
  #define GPIO_I2C_PINS_MAX 8
  #define GPIO_I2C_PINS_ALL 0xff

  #define GPIO_I2C_PCA9554_PIN_COUNT 8
  #define GPIO_I2C_PCA9554_PINS_MASK 0xff

  #define GPIO_I2C_PCA9554_ADDR(addr) (GPIO_I2C_PCA9554_ADDR_BASE | ((addr) & GPIO_I2C_PCA9554_ADDR_MASK))
  #define GPIO_I2C_PCA9554_ADDR_BASE 0x20
  #define GPIO_I2C_PCA9554_ADDR_MASK 0x07

  enum gpio_i2c_type {
    GPIO_I2C_TYPE_NONE,
    GPIO_I2C_TYPE_PCA9534,
    GPIO_I2C_TYPE_PCA9554,  /* With weak internal pull-up on inputs */
  };

  struct gpio_i2c_options {
    enum gpio_i2c_type type;
    i2c_port_t port;
    uint8_t addr;
    gpio_pin_t int_pin;

    /* XXX: no support for multiple i2c_gpio_dev sharing the same int_pin */
  };

  struct gpio_i2c_dev;

  int gpio_i2c_new(struct gpio_i2c_dev **devp, const struct gpio_i2c_options *options);

  const struct gpio_i2c_options *gpio_i2c_options(struct gpio_i2c_dev *dev);
#endif

/* Return gpio_out_pins bitmask for numbered pin, or 0 if invalid */
gpio_pins_t gpio_host_pin(gpio_pin_t pin);

typedef void (*gpio_interrupt_func_t)(gpio_pins_t pins, void *arg);

struct gpio_options {
  enum gpio_type type;

  union {
  #if GPIO_I2C_ENABLED
     struct gpio_i2c_dev *i2c_dev;
  #endif
  };

  // bitmask of input pins to enable
  gpio_pins_t in_pins;

  // bitmask of outputs pins to enable
  gpio_pins_t out_pins;

  // invert output for pins
  gpio_pins_t inverted_pins;

  // enable input interrupts for pins
  gpio_pins_t interrupt_pins;

  // interrupt handler
  gpio_interrupt_func_t interrupt_func;
  void *interrupt_arg;

  // for i2c devices
  TickType_t i2c_timeout;
};

/* Setup input interrupt handler */
int gpio_intr_init();

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
