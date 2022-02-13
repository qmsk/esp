#pragma once

#include <esp_timer.h>

struct stats_gauge {
  uint64_t reset, update;
  unsigned value;
  unsigned min, max;
};

static inline void stats_gauge_init(struct stats_gauge *gauge)
{
  gauge->reset = esp_timer_get_time();
  gauge->value = 0;
  gauge->min = -1;
  gauge->max = 0;
}

static inline struct stats_gauge stats_gauge_copy(const struct stats_gauge *gauge)
{
  // TODO: locking for concurrent stats updates
  return *gauge;
}

static inline void stats_gauge_sample(struct stats_gauge *gauge, unsigned value)
{
  gauge->update = esp_timer_get_time();
  gauge->value = value;

  if (value < gauge->min) {
    gauge->min = value;
  }

  if (value > gauge->max) {
    gauge->max = value;
  }
}

static inline float stats_gauge_seconds_passed(const struct stats_gauge *gauge)
{
  if (gauge->update > gauge->reset) {
    return ((float)(gauge->update - gauge->reset)) / 1000000.0f;
  } else {
    return 0.0f;
  }
}
