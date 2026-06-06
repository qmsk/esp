#include <stats_counter.h>

struct stats_counter_metrics stats_counter_diff_metrics(const struct stats_counter *old, const struct stats_counter *new)
{
  if ((!old->update || new->reset > old->reset) && !new->update) {
    // increasing interval with zero rate until first update
    return (struct stats_counter_metrics) {
      .interval = (float)(esp_timer_get_time() - new->reset) / 1000000.0f,
    };
  } else if ((!old->update || new->reset > old->reset) && (new->update > new->reset)) {
    // initial update
    return (struct stats_counter_metrics) {
      .interval = (float)(new->update - new->reset) / 1000000.0f,
      .rate = ((float) (new->count)) / ((float)(new->update - new->reset) / 1000000.0f),
    };
  } else if ((old->update && old->reset == new->reset) && new->update == old->update) {
    // increasing interval with zero rate if updates stop
    return (struct stats_counter_metrics) {
      .interval = (float)(esp_timer_get_time() - old->update) / 1000000.0f,
    };
  } else if ((old->update && old->reset == new->reset) && new->update > old->update) {
    // incremental update
    return (struct stats_counter_metrics) {
      .interval = (float)(new->update - old->update) / 1000000.0f,
      .rate = ((float) (new->count - old->count)) / ((float)(new->update - old->update) / 1000000.0f),
    };
  } else {
    // ???
    return (struct stats_counter_metrics) {};
  }
}

struct stats_counter_metrics stats_counter_metrics_average(const struct stats_counter_metrics *old, const struct stats_counter_metrics *new)
{
  if (old->interval && new->interval) {
    return (struct stats_counter_metrics) {
      .interval = (old->interval + new->interval) / 2,
      .rate = (old->rate + new->rate) / 2,
    };
  } else if (old->interval && !new->interval) {
    return *old;
  } else if (!old->interval && new->interval) {
    return *new;
  } else {
    return (struct stats_counter_metrics) {};
  }
}
