#pragma once

#include <stats.h>

extern struct stats_timer leds_stats_artnet_loop;
extern struct stats_timer leds_stats_artnet_test;
extern struct stats_timer leds_stats_artnet_set;
extern struct stats_timer leds_stats_artnet_update;

void leds_stats_init();
