#ifndef _STATS_H_
#define _STATS_H_

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>


#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct stats_counter {
  uint32_t tick;
  uint32_t count;
};

static inline void stats_counter_init(struct stats_counter *counter)
{
  counter->tick = xTaskGetTickCount();
  counter->count = 0;
}

static inline bool stats_counter_zero(const struct stats_counter *counter)
{
  return counter->count == 0;
}

static inline void stats_counter_increment(struct stats_counter *counter)
{
  counter->tick = xTaskGetTickCount();
  counter->count++;
}

static inline struct stats_counter stats_counter_copy(const struct stats_counter *counter)
{
  // TODO: locking for concurrent stats updates
  return *counter;
}

static inline unsigned stats_counter_ticks_passed(const struct stats_counter *counter)
{
  return (xTaskGetTickCount() - counter->tick);
}
static inline unsigned stats_counter_milliseconds_passed(const struct stats_counter *counter)
{
  return ((xTaskGetTickCount() - counter->tick) * portTICK_PERIOD_MS);
}

struct stats_gauge {
  unsigned val;
  unsigned min, max;
};

static inline void stats_gauge_init(struct stats_gauge *gauge)
{
  gauge->val = 0;
  gauge->min = -1;
  gauge->max = 0;
}

static inline struct stats_gauge stats_gauge_copy(const struct stats_gauge *gauge)
{
  // TODO: locking for concurrent stats updates
  return *gauge;
}

static inline void stats_gauge_sample(struct stats_gauge *gauge, unsigned val)
{
  gauge->val = val;

  if (val < gauge->min) {
    gauge->min = val;
  }

  if (val > gauge->max) {
    gauge->max = val;
  }
}

#endif
