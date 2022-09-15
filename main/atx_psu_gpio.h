#pragma once

#include "atx_psu_config.h"

#include <atx_psu.h>

int init_atx_psu_gpio(const struct atx_psu_config *config);
void config_atx_psu_gpio(const struct atx_psu_config *config, struct atx_psu_options *options);
