#include "leds_stats.h"

struct stats_timer leds_stats_artnet_loop;
struct stats_timer leds_stats_artnet_test;
struct stats_timer leds_stats_artnet_set;
struct stats_timer leds_stats_artnet_update;

struct stats_counter leds_stats_artnet_timeout;

void init_leds_artnet_stats(struct leds_artnet_stats *stats)
{
  stats_timer_init(&stats->loop);
  stats_timer_init(&stats->test);
  stats_timer_init(&stats->set);
  stats_timer_init(&stats->update);

  stats_counter_init(&stats->timeout);
}

void init_leds_stats()
{
  for (unsigned i = 0; i < LEDS_COUNT; i++) {
    init_leds_artnet_stats(&leds_artnet_stats[i]);
  }
}
