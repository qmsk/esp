#pragma once

#include <gpio.h>

/* gpio_host.c */
int gpio_host_setup(const struct gpio_options *options);
void gpio_host_clear(const struct gpio_options *options);
void gpio_host_set(const struct gpio_options *options, gpio_pins_t pins);
void gpio_host_set_all(const struct gpio_options *options);
