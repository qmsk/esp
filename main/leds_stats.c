#include "leds_stats.h"

struct leds_stats leds_stats[LEDS_COUNT];

void init_leds_stats()
{
  for (unsigned i = 0; i < LEDS_COUNT; i++) {
    struct leds_stats *stats = &leds_stats[i];

    stats_timer_init(&stats->loop);

    stats_timer_init(&stats->test);

    stats_timer_init(&stats->artnet);
    stats_counter_init(&stats->artnet_timeout);

    stats_timer_init(&stats->update);
  }
}
