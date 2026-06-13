#pragma once

#include "leds.h"

#include <stats.h>
#include <i2s_out_stats.h>

struct leds_sequence_stats {
  struct stats_timer read;
  struct stats_counter skip;
};

struct leds_stats {
  struct stats_timer loop;

  struct stats_timer test;

  struct stats_timer artnet;
  struct stats_counter artnet_timeout;
  struct stats_counter artnet_sync;

  struct stats_counter sync_none;
  struct stats_counter sync_timeout;
  struct stats_counter sync_missed;
  struct stats_counter sync_full;

  struct stats_timer sequence;

  struct stats_timer static_;
  
  struct stats_timer output;
  struct stats_counter update;
  struct stats_counter update_timeout;

};

extern struct leds_sequence_stats leds_sequence_stats;
extern struct leds_stats leds_stats[LEDS_COUNT];

void init_leds_stats();

struct i2s_out_stats get_leds_i2s_out_stats(unsigned port);
void reset_leds_i2s_out_stats();
