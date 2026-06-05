#pragma once

#include <esp_timer.h>

#include <stdbool.h>

#define WITH_STATS_TIMER(timer) \
  for (stats_timer_start_t _stats_timer_start = stats_timer_start(timer); _stats_timer_start; stats_timer_stop(timer, &_stats_timer_start))

struct stats_timer {
  uint64_t reset, update;
  uint32_t count;
  uint64_t total; // us
};

typedef uint64_t stats_timer_start_t;

static inline void stats_timer_init(struct stats_timer *timer)
{
  timer->reset = esp_timer_get_time();
  timer->update = 0;
  timer->count = 0;
  timer->total = 0;
}

static inline struct stats_timer stats_timer_copy(const struct stats_timer *timer)
{
  // TODO: locking for concurrent stats updates?
  return *timer;
}

// IRAM-safe
static inline void stats_timer_update(struct stats_timer *timer, stats_timer_start_t start)
{
  assert(start);
  
  uint64_t stop = esp_timer_get_time();

  timer->update = esp_timer_get_time();
  timer->count += 1;
  timer->total += stop - start;
}

static inline stats_timer_start_t stats_timer_start(struct stats_timer *timer)
{
  return esp_timer_get_time();
}

static inline void stats_timer_stop(struct stats_timer *timer, stats_timer_start_t *startp)
{
  stats_timer_update(timer, *startp);

  *startp = 0;
}

static inline float stats_timer_seconds_passed(const struct stats_timer *timer)
{
  if (timer->update > timer->reset) {
    return ((float)(timer->update - timer->reset)) / 1000000.0f;
  } else {
    return 0.0f;
  }
}

static inline float stats_timer_average_rate(const struct stats_timer *timer)
{
  if (timer->update > timer->reset) {
    return ((float) timer->count) / ((float)(timer->update - timer->reset) / 1000000.0f);
  } else {
    return 0.0f;
  }
}

static inline float stats_timer_total_seconds(const struct stats_timer *timer)
{
  if (timer->count) {
    return ((float) timer->total) / 1000.0f / 1000.0f;
  } else {
    return 0.0f;
  }
}

static inline float stats_timer_average_seconds(const struct stats_timer *timer)
{
  if (timer->count) {
    return ((float) timer->total) / ((float) timer->count) / 1000000.0f;
  } else {
    return 0.0f;
  }
}

/* Return portion total time over seconds passed, as a ratio 0..1  */
static inline float stats_timer_utilization(const struct stats_timer *timer)
{
  if (timer->update > timer->reset) {
    return ((float) timer->total) / ((float)(timer->update - timer->reset));
  } else {
    return 0.0f;
  }
}


struct stats_timer_metrics {
  float interval;
  float rate;
  float util;
};

/*
 * Compute metrics between old -> new.
 *
 * Returns zero values if new is invalid.
 * Returns values from new if old is invalid.
 */
struct stats_timer_metrics stats_timer_diff_metrics(const struct stats_timer *old, const struct stats_timer *new);

/*
 * Compute moving averge between old + new / 2.
 */
struct stats_timer_metrics stats_timer_metrics_average(const struct stats_timer_metrics *old, const struct stats_timer_metrics *new);

