#pragma once

#include "leds.h"

#include <stats.h>

struct leds_sequence_stats {
  struct stats_timer read;
  struct stats_counter skip;
};

struct leds_stats {
  struct stats_timer loop;

  struct stats_timer test;

  struct stats_timer artnet;
  struct stats_counter artnet_timeout;

  struct stats_counter sync_timeout;

  struct stats_timer sequence;

  struct stats_timer update;
  struct stats_counter update_timeout;
};

extern struct leds_sequence_stats leds_sequence_stats;
extern struct leds_stats leds_stats[LEDS_COUNT];

void init_leds_stats();
