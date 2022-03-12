#pragma once

#include <stats.h>

extern struct stats_timer leds_stats_artnet_loop;
extern struct stats_timer leds_stats_artnet_test;
extern struct stats_timer leds_stats_artnet_set;
extern struct stats_timer leds_stats_artnet_update;

extern struct stats_counter leds_stats_artnet_timeout;

void leds_stats_init();
