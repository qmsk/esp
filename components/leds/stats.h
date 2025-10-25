#pragma once

#include <leds_stats.h>

extern struct leds_interface_stats leds_interface_stats;

struct leds_interface_i2s_stats *leds_interface_i2s_stats(enum leds_interface interface);
