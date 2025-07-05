#include "leds_artnet.h"
#include "leds_sequence.h"
#include "leds_state.h"
#include "leds_stats.h"
#include "leds_task.h"
#include "leds_test.h"

#include "tasks.h"
#include "user.h"

#include <logging.h>

static TickType_t leds_update_wait(struct leds_state *state)
{
  if (state->config->update_timeout && state->update_tick) {
    // refresh valid date
    return state->update_tick + state->config->update_timeout / portTICK_PERIOD_MS;
  }

  return 0;
}


static bool leds_update_active(struct leds_state *state)
{
  // not configured
  if (!state->config->update_timeout) {
    return false;
  }

  // not initialized
  if (!state->update_tick) {
    return false;
  }

  return xTaskGetTickCount() >= state->update_tick + state->config->update_timeout / portTICK_PERIOD_MS;
}

static EventBits_t leds_task_wait(struct leds_state *state)
{
  // XXX: how to handle tick overflow? with uint32_t TickType @ 100Hz, this will likely bug out after ~497 days of uptime...
  TickType_t tick;

  // first tick to wait until
  TickType_t wait_tick = portMAX_DELAY;

  if ((tick = leds_update_wait(state))) {
    if (tick < wait_tick) {
      wait_tick = tick;
    }
  }

  if (state->test && (tick = leds_test_wait(state))) {
    if (tick < wait_tick) {
      wait_tick = tick;
    }
  }

  if (state->artnet && (tick = leds_artnet_wait(state))) {
    if (tick < wait_tick) {
      wait_tick = tick;
    }
  }

  if (state->sequence && (tick = leds_sequence_wait(state))) {
    if (tick < wait_tick) {
      wait_tick = tick;
    }
  }

  // how long to wait for
  TickType_t wait_ticks = portMAX_DELAY;
  tick = xTaskGetTickCount();

  if (wait_tick == portMAX_DELAY) {
    wait_ticks = portMAX_DELAY;
  } else if (wait_tick <= tick) {
    // late, catch up
    wait_ticks = 0; // immediate
  } else {
    wait_ticks = wait_tick - tick;
  }

  LOG_DEBUG("leds%d: wait_tick=%d wait_ticks=%d", state->index + 1, wait_tick, wait_ticks);

  const bool clear_on_exit = true;
  const bool wait_for_all_bits = false;
  EventBits_t event_bits = xEventGroupWaitBits(state->event_group, LEDS_EVENT_BITS, clear_on_exit, wait_for_all_bits, wait_ticks);

  LOG_DEBUG("leds%d: test=%d artnet_dmx=%d artnet_sync=%d sequence=%d", state->index + 1,
    !!(event_bits & (1 << LEDS_EVENT_TEST_BIT)),
    !!(event_bits & (1 << LEDS_EVENT_ARTNET_DMX_BIT)),
    !!(event_bits & (1 << LEDS_EVENT_ARTNET_SYNC_BIT)),
    !!(event_bits & (1 << LEDS_EVENT_SEQUENCE_BIT))
  );

  return event_bits;
}

static void leds_main(void *ctx)
{
  struct leds_state *state = ctx;
  struct leds_stats *stats = &leds_stats[state->index];

  for(struct stats_timer_sample loop_sample;; stats_timer_stop(&stats->loop, &loop_sample)) {
    EventBits_t event_bits = leds_task_wait(state);
    enum user_activity update_activity = 0;
    bool update_timeout = false;

    loop_sample = stats_timer_start(&stats->loop);

    if (state->sequence && leds_sequence_active(state, event_bits)) {
      WITH_STATS_TIMER(&stats->sequence) {
        if (leds_sequence_update(state, event_bits)) {
          update_activity = USER_ACTIVITY_LEDS_SEQUENCE;
        }
      }
    }

    if (state->artnet && leds_artnet_active(state, event_bits)) {
      WITH_STATS_TIMER(&stats->artnet) {
        switch (leds_artnet_update(state, event_bits)) {
          case 0:
            break;
          
          case LEDS_ARTNET_UPDATE:
            update_activity = USER_ACTIVITY_LEDS_ARTNET;
            break;
          
          case LEDS_ARTNET_UPDATE_TIMEOUT:
            update_activity = USER_ACTIVITY_LEDS_ARTNET_TIMEOUT;
            break;
          
          default:
            LOG_ERROR("leds_artnet_update");
        }
      }
    }

    if (state->test && leds_test_active(state, event_bits)) {
      WITH_STATS_TIMER(&stats->test) {
        if (leds_test_update(state, event_bits)) {
          update_activity = USER_ACTIVITY_LEDS_TEST;
        }
      }
    }

    if (leds_update_active(state)) {
      // update without activity
      update_timeout = true;

      LOG_DEBUG("update timeout");

      stats_counter_increment(&stats->update_timeout);
    }

    if (update_activity || update_timeout) {
      state->update_tick = xTaskGetTickCount();

      WITH_STATS_TIMER(&stats->update) {
        if (update_leds(state, update_activity)) {
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
  if (!state->event_group) {
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
