#pragma once

#include <gpio.h>

#if CONFIG_IDF_TARGET_ESP8266
  #include <driver/gpio.h>
#else
  #include <hal/gpio_types.h>
#endif

/* gpio_host.c */
#if GPIO_I2C_ENABLED
  int gpio_host_setup_intr_pin(const struct gpio_options *options, gpio_pin_t pin, gpio_int_type_t int_type);
#endif

int gpio_host_setup(const struct gpio_options *options);
int gpio_host_setup_input(const struct gpio_options *options, gpio_pins_t pins);
int gpio_host_get(const struct gpio_options *options, gpio_pins_t *pins);
int gpio_host_setup_output(const struct gpio_options *options, gpio_pins_t pins);
int gpio_host_clear(const struct gpio_options *options);
int gpio_host_set(const struct gpio_options *options, gpio_pins_t pins);
int gpio_host_set_all(const struct gpio_options *options);

/* gpio_host_intr.c */
int gpio_host_intr_init();
void gpio_host_intr_setup_pin(const struct gpio_options *options, gpio_pin_t gpio);

#if !CONFIG_IDF_TARGET_ESP8266
  int gpio_host_intr_core();
#endif

/* gpio_i2c_pc54xx.c */
int gpio_i2c_pc54xx_setup(const struct gpio_options *options);
int gpio_i2c_pc54xx_setup_input(const struct gpio_options *options, gpio_pins_t pins);
int gpio_i2c_pc54xx_get(const struct gpio_options *options, gpio_pins_t *pins);
int gpio_i2c_pc54xx_setup_output(const struct gpio_options *options, gpio_pins_t pins);
int gpio_i2c_pc54xx_clear(const struct gpio_options *options);
int gpio_i2c_pc54xx_set(const struct gpio_options *options, gpio_pins_t pins);
int gpio_i2c_pc54xx_set_all(const struct gpio_options *options);
