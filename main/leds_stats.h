#pragma once

#include "leds.h"

#include <stats.h>

struct leds_stats {
  struct stats_timer loop;

  struct stats_timer test;

  struct stats_timer artnet;
  struct stats_counter artnet_timeout;

  struct stats_timer sequence;
  struct stats_counter sequence_skip;

  struct stats_timer update;
};

extern struct leds_stats leds_stats[LEDS_COUNT];

void init_leds_stats();
