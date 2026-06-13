#include "leds_stats.h"
#include "leds_i2s.h"

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

    stats_timer_init(&stats->update);
    stats_timer_init(&stats->loop);
    stats_timer_init(&stats->test);
    stats_timer_init(&stats->artnet);
    stats_timer_init(&stats->sequence);
    stats_timer_init(&stats->static_);
    stats_timer_init(&stats->output);

    stats_counter_init(&stats->artnet_timeout);
    stats_counter_init(&stats->artnet_sync);
    stats_counter_init(&stats->sync_none);
    stats_counter_init(&stats->sync_timeout);
    stats_counter_init(&stats->sync_missed);
    stats_counter_init(&stats->sync_full);
    stats_counter_init(&stats->update_timeout);
  }
}

struct i2s_out_stats get_leds_i2s_out_stats(unsigned port)
{
  struct i2s_out *i2s_out = NULL;
  
  if (port < LEDS_I2S_INTERFACE_COUNT) {
    i2s_out = leds_i2s_out[port];
  }

  if (i2s_out) {
    return i2s_out_stats(i2s_out);
  } else {
    return (struct i2s_out_stats) {};
  }
}

void reset_leds_i2s_out_stats()
{
  struct i2s_out *i2s_out = NULL;
  
  for (unsigned port = 0; port < LEDS_I2S_INTERFACE_COUNT; port++) {
    if ((i2s_out = leds_i2s_out[port])) {
      i2s_out_reset_stats(i2s_out);
    }
  }
}
