#include "leds_artnet.h"
#include "leds_state.h"
#include "leds_stats.h"
#include "leds_task.h"
#include "leds_test.h"

#include "tasks.h"

#include <logging.h>

static EventBits_t leds_task_wait(struct leds_state *state)
{
  TickType_t wait_tick = portMAX_DELAY;
  const bool clear_on_exit = true;
  const bool wait_for_all_bits = false;

  if (state->test) {
    TickType_t test_tick = leds_test_wait(state);

    if (test_tick && test_tick < wait_tick) {
      wait_tick = test_tick;
    }
  }

  if (state->artnet) {
    TickType_t artnet_tick = leds_artnet_wait(state);

    if (artnet_tick && artnet_tick < wait_tick) {
      wait_tick = artnet_tick;
    }
  }

  // how long to wait
  TickType_t tick = xTaskGetTickCount(), wait_ticks = portMAX_DELAY;

  if (wait_tick == portMAX_DELAY) {
    wait_ticks = portMAX_DELAY;
  } else if (wait_tick <= tick) {
    // late, catch up
    wait_ticks = 0; // immediate
  } else {
    wait_ticks = wait_tick - tick;
  }

  LOG_DEBUG("leds%d: wait_tick=%d wait_ticks=%d", state->index + 1, wait_tick, wait_ticks);

  EventBits_t event_bits = xEventGroupWaitBits(state->event_group, ARTNET_OUTPUT_EVENT_INDEX_BITS | ARTNET_OUTPUT_EVENT_FLAG_BITS, clear_on_exit, wait_for_all_bits, wait_ticks);

  LOG_DEBUG("leds%d: artnet=%04x artnet_sync=%d test=%d", state->index + 1,
    (event_bits & ARTNET_OUTPUT_EVENT_INDEX_BITS),
    !!(event_bits & (1 << ARTNET_OUTPUT_EVENT_SYNC_BIT)),
    !!(event_bits & (1 << LEDS_EVENT_TEST_BIT))
  );

  return event_bits;
}

static void leds_main(void *ctx)
{
  struct leds_state *state = ctx;
  struct leds_stats *stats = &leds_stats[state->index];

  for(struct stats_timer_sample loop_sample;; stats_timer_stop(&stats->loop, &loop_sample)) {
    EventBits_t event_bits = leds_task_wait(state);
    bool update = false;

    loop_sample = stats_timer_start(&stats->loop);

    if (leds_artnet_active(state, event_bits)) {
      WITH_STATS_TIMER(&stats->artnet) {
        if (leds_artnet_update(state, event_bits)) {
          update = true;
        }
      }
    }

    if (leds_test_active(state, event_bits)) {
      WITH_STATS_TIMER(&stats->test) {
        if (leds_test_update(state, event_bits)) {
          update = true;
        }
      }
    }

    if (update) {
      WITH_STATS_TIMER(&stats->update) {
        if (update_leds(state)) {
          LOG_WARN("leds%d: update_leds", state->index + 1);
          continue;
        }
      }
    }
  }
}

int init_leds_task(struct leds_state *state, const struct leds_config *config)
{
  if (!(state->event_group = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  return 0;
}

int start_leds_task(struct leds_state *state, const struct leds_config *config)
{
  struct task_options task_options = {
    .main       = leds_main,
    .name_fmt   = LEDS_TASK_NAME_FMT,
    .stack_size = LEDS_TASK_STACK,
    .arg        = state,
    .priority   = LEDS_TASK_PRIORITY,
    .handle     = &state->task,
    .affinity   = LEDS_TASK_AFFINITY,
  };

  if (start_taskf(task_options, state->index + 1)) {
    LOG_ERROR("start_taskf");
    return -1;
  } else {
    LOG_INFO("leds%d: start task=%p", state->index + 1, state->task);
  }

  return 0;
}

void notify_leds_task(struct leds_state *state, EventBits_t bits)
{
  if (!state->task) {
    LOG_WARN("leds%d: not initialized", state->index + 1);
    return;
  }

  xEventGroupSetBits(state->event_group, bits);
}

void notify_leds_tasks(EventBits_t bits)
{
  for (int i = 0; i < LEDS_COUNT; i++)
  {
    struct leds_state *state = &leds_states[i];
    const struct leds_config *config = &leds_configs[i];

    if (!config->enabled) {
      continue;
    }

    notify_leds_task(state, bits);
  }
}
