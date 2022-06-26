#pragma once

#include <gpio.h>

/* gpio_host.c */
int gpio_host_setup(const struct gpio_options *options);
int gpio_host_setup_input(const struct gpio_options *options, gpio_pins_t pins);
int gpio_host_get(const struct gpio_options *options, gpio_pins_t *pins);
int gpio_host_setup_output(const struct gpio_options *options, gpio_pins_t pins);
int gpio_host_clear(const struct gpio_options *options);
int gpio_host_set(const struct gpio_options *options, gpio_pins_t pins);
int gpio_host_set_all(const struct gpio_options *options);

/* gpio_i2c_pc54xx.c */
int gpio_i2c_pc54xx_setup(const struct gpio_options *options);
int gpio_i2c_pc54xx_setup_input(const struct gpio_options *options, gpio_pins_t pins);
int gpio_i2c_pc54xx_get(const struct gpio_options *options, gpio_pins_t *pins);
int gpio_i2c_pc54xx_setup_output(const struct gpio_options *options, gpio_pins_t pins);
int gpio_i2c_pc54xx_clear(const struct gpio_options *options);
int gpio_i2c_pc54xx_set(const struct gpio_options *options, gpio_pins_t pins);
int gpio_i2c_pc54xx_set_all(const struct gpio_options *options);
