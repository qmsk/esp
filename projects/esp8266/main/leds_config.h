#pragma once

#include <config.h>

#define LEDS_COUNT 4

extern const struct configtab *leds_configtabs[LEDS_COUNT];

extern const struct config_enum leds_interface_enum[];
extern const struct config_enum leds_protocol_enum[];
extern const struct config_enum leds_rate_enum[];
extern const struct config_enum leds_gpio_mode_enum[];
extern const struct config_enum leds_artnet_mode_enum[];
extern const struct config_enum leds_color_parameter_enum[];
extern const struct config_enum leds_test_mode_enum[];
