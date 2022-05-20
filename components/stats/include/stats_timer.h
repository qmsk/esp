#pragma once

#include <esp_timer.h>

#include <stdbool.h>

struct stats_timer {
  uint64_t reset, update;
  uint32_t count;
  uint64_t total; // us
};

struct stats_timer_sample {
  uint64_t start;
  bool running;
};

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

static inline void stats_timer_update(struct stats_timer *timer, uint32_t count, uint64_t interval)
{
  timer->update = esp_timer_get_time();
  timer->count += count;
  timer->total += interval;
}

static inline struct stats_timer_sample stats_timer_start(struct stats_timer *timer)
{
  return (struct stats_timer_sample) {
    .start    = esp_timer_get_time(),
    .running  = true,
  };
}

static inline void stats_timer_stop(struct stats_timer *timer, struct stats_timer_sample *sample)
{
  uint64_t stop = esp_timer_get_time();

  stats_timer_update(timer, 1, stop - sample->start);

  sample->running = false;
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

#define WITH_STATS_TIMER(timer) \
  for (struct stats_timer_sample _stats_timer_sample = stats_timer_start(timer); _stats_timer_sample.running; stats_timer_stop(timer, &_stats_timer_sample))
