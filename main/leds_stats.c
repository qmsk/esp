#include "leds_stats.h"

struct leds_sequence_stats leds_sequence_stats;
struct leds_stats leds_stats[LEDS_COUNT];

void init_leds_sequence_stats(struct leds_sequence_stats *stats)
{
  stats_timer_init(&stats->read);
  stats_counter_init(&stats->skip);
}

void init_leds_stats()
{
  init_leds_sequence_stats(&leds_sequence_stats);

  for (unsigned i = 0; i < LEDS_COUNT; i++) {
    struct leds_stats *stats = &leds_stats[i];

    stats_timer_init(&stats->loop);
    stats_timer_init(&stats->test);
    stats_timer_init(&stats->artnet);
    stats_timer_init(&stats->sequence);
    stats_timer_init(&stats->update);

    stats_counter_init(&stats->artnet_timeout);
  }
}
