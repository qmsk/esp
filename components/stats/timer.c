#include <stats_timer.h>

struct stats_timer_metrics stats_timer_diff_metrics(const struct stats_timer *old, const struct stats_timer *new)
{
    if ((!old->update || new->reset > old->reset) && !new->update) {
    // increasing interval with zero rate until first update
    return (struct stats_timer_metrics) {
      .interval = (float)(esp_timer_get_time() - new->reset) / 1000000.0f,
    };
  } else if ((!old->update || new->reset > old->reset) && (new->update > new->reset)) {
    // initial update
    return (struct stats_timer_metrics) {
      .interval = (float)(new->update - new->reset) / 1000000.0f,
      .rate = ((float) (new->count)) / ((float)(new->update - new->reset) / 1000000.0f),
      .util = ((float) (new->total)) / ((float)(new->update - new->reset)),
    };
  } else if ((old->update && old->reset == new->reset) && new->update == old->update) {
    // increasing interval with zero rate if updates stop
    return (struct stats_timer_metrics) {
      .interval = (float)(esp_timer_get_time() - old->update) / 1000000.0f,
    };
  } else if ((old->update && old->reset == new->reset) && new->update > old->update) {
    // incremental update
    return (struct stats_timer_metrics) {
      .interval = (float)(new->update - old->update) / 1000000.0f,
      .rate = ((float) (new->count - old->count)) / ((float)(new->update - old->update) / 1000000.0f),
      .util = ((float) (new->total - old->total)) / ((float)(new->update - old->update)),
    };
  } else {
    // ???
    return (struct stats_timer_metrics) {};
  }
}

struct stats_timer_metrics stats_timer_metrics_average(const struct stats_timer_metrics *old, const struct stats_timer_metrics *new)
{
  if (old->interval && new->interval) {
    return (struct stats_timer_metrics) {
      .interval = (old->interval + new->interval) / 2,
      .rate = (old->rate + new->rate) / 2,
      .util = (old->util + new->util) / 2,
    };
  } else if (old->interval && !new->interval) {
    return *old;
  } else if (!old->interval && new->interval) {
    return *new;
  } else {
    return (struct stats_timer_metrics) {};
  }
}
