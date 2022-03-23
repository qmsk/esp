#include "leds_stats.h"

struct stats_timer leds_stats_artnet_loop;
struct stats_timer leds_stats_artnet_test;
struct stats_timer leds_stats_artnet_set;
struct stats_timer leds_stats_artnet_update;

struct stats_counter leds_stats_artnet_timeout;

void leds_stats_init()
{
  stats_timer_init(&leds_stats_artnet_loop);
  stats_timer_init(&leds_stats_artnet_test);
  stats_timer_init(&leds_stats_artnet_set);
  stats_timer_init(&leds_stats_artnet_update);

  stats_counter_init(&leds_stats_artnet_timeout);
}

void leds_stats_reset()
{
  // same same
  leds_stats_init();
}
