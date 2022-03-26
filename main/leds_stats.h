#pragma once

#include "leds.h"

#include <stats.h>

struct leds_artnet_stats {
  struct stats_timer loop;
  struct stats_timer test;
  struct stats_timer set;
  struct stats_timer update;

  struct stats_counter timeout;
} leds_artnet_stats[LEDS_COUNT];

void init_leds_artnet_stats(struct leds_artnet_stats *stats);
void init_leds_stats();
