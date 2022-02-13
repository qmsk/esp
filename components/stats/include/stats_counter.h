#pragma once

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