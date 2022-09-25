#pragma once

#include <config.h>
#include <gpio.h>

// references to esp / i2c-gpio devcies
extern const struct config_enum gpio_type_enum[];

int set_gpio_type(struct gpio_options *options, int type);
