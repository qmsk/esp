#pragma once

#include "atx_psu_config.h"

#include <atx_psu.h>

int config_atx_psu_gpio(const struct atx_psu_config *config, struct gpio_options *gpio_options);
