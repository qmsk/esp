#include <stats_timer.h>

struct stats_timer_metrics stats_timer_diff_metrics(const struct stats_timer *old, const struct stats_timer *new)
{
  if ((old && old->update > 0) && (new && new->update > old->update)) {
    return (struct stats_timer_metrics) {
      .interval = (float)(new->update - old->update) / 1000000.0f,
      .rate = ((float) (new->count - old->count)) / ((float)(new->update - old->update) / 1000000.0f),
      .util = ((float) (new->total - old->total)) / ((float)(new->update - old->update)),
    };
  } else if (!(old && old->update) && (new && new->update > new->reset)) {
    return (struct stats_timer_metrics) {
      .interval = (float)(new->update - new->reset) / 1000000.0f,
      .rate = ((float) (new->count)) / ((float)(new->update - new->reset) / 1000000.0f),
      .util = ((float) (new->total)) / ((float)(new->update - new->reset)),
    };
  } else {
    return (struct stats_timer_metrics) {};
  }
}

struct stats_timer_metrics stats_timer_metrics_average(const struct stats_timer_metrics *old, const struct stats_timer_metrics *new)
{
  if ((old && old->interval) && (new && new->interval)) {
    return (struct stats_timer_metrics) {
      .interval = (old->interval + new->interval) / 2,
      .rate = (old->rate + new->rate) / 2,
      .util = (old->util + new->util) / 2,
    };
  } else if ((old && old->interval) && !(new && new->interval)) {
    return *old;
  } else if (!(old && old->interval) && (new && new->interval)) {
    return *new;
  } else {
    return (struct stats_timer_metrics) {};
  }
}
