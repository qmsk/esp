#include <i2s_out_stats.h>

#include "i2s_out.h"

void i2s_out_stats_reset(struct i2s_out_stats *stats)
{
  stats_timer_init(&stats->out_timer);
}

struct i2s_out_stats i2s_out_stats_copy(struct i2s_out_stats *stats)
{
    return (struct i2s_out_stats) {
        .out_timer = stats_timer_copy(&stats->out_timer),
    };
}
