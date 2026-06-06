#pragma once

#include <esp_timer.h>

#include <stdbool.h>

struct stats_counter {
  uint64_t reset, update;
  uint32_t count;
};

static inline void stats_counter_init(struct stats_counter *counter)
{
  counter->reset = esp_timer_get_time();
  counter->count = 0;
}

static inline bool stats_counter_zero(const struct stats_counter *counter)
{
  return counter->count == 0;
}

static inline void stats_counter_increment(struct stats_counter *counter)
{
  counter->update = esp_timer_get_time();
  counter->count++;
}

static inline void stats_counter_add(struct stats_counter *counter, uint32_t value)
{
  counter->update = esp_timer_get_time();
  counter->count += value;
}

static inline struct stats_counter stats_counter_copy(const struct stats_counter *counter)
{
  // TODO: locking for concurrent stats updates
  return *counter;
}

static inline float stats_counter_seconds_passed(const struct stats_counter *counter)
{
  if (counter->update > counter->reset) {
    return ((float)(counter->update - counter->reset)) / 1000000.0f;
  } else {
    return 0.0f;
  }
}

struct stats_counter_metrics {
  float interval;
  float rate;
};

struct stats_counter_metrics stats_counter_diff_metrics(const struct stats_counter *old, const struct stats_counter *new);
struct stats_counter_metrics stats_counter_metrics_average(const struct stats_counter_metrics *old, const struct stats_counter_metrics *new);
