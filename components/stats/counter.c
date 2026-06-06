#include <stats_counter.h>

struct stats_counter_metrics stats_counter_diff_metrics(const struct stats_counter *old, const struct stats_counter *new)
{
  if ((old && old->update > 0) && (new && new->update == old->update)) {
    // increasing interval if updates stop
    return (struct stats_counter_metrics) {
      .interval = (float)(esp_timer_get_time() - old->update) / 1000000.0f,
    };
  } else if ((old && old->update > 0) && (new && new->update > old->update)) {
    // incremental update
    return (struct stats_counter_metrics) {
      .interval = (float)(new->update - old->update) / 1000000.0f,
      .rate = ((float) (new->count - old->count)) / ((float)(new->update - old->update) / 1000000.0f),
    };
  } else if (!(old && old->update) && (new && new->update > new->reset)) {
    // initial update
    return (struct stats_counter_metrics) {
      .interval = (float)(new->update - new->reset) / 1000000.0f,
      .rate = ((float) (new->count)) / ((float)(new->update - new->reset) / 1000000.0f),
    };
  } else if (!(old && old->update) && (new && new->reset)) {
    // increasing interval until first update
    return (struct stats_counter_metrics) {
      .interval = (float)(esp_timer_get_time() - new->reset) / 1000000.0f,
    };
  } else {
    return (struct stats_counter_metrics) {};
  }
}

struct stats_counter_metrics stats_counter_metrics_average(const struct stats_counter_metrics *old, const struct stats_counter_metrics *new)
{
  if ((old && old->interval) && new) {
    return (struct stats_counter_metrics) {
      .interval = (old->interval + new->interval) / 2,
      .rate = (old->rate + new->rate) / 2,
    };
  } else if ((old && old->interval) && !(new && new->interval)) {
    return *old;
  } else if (!(old && old->interval) && (new && new->interval)) {
    return *new;
  } else {
    return (struct stats_counter_metrics) {};
  }
}
