#include "leds_status.h"
#include "leds_state.h"
#include "leds_stats.h"
#include "leds_test.h"
#include "leds_artnet.h"
#include "leds.h"

const struct config_enum leds_update_state_enum[] = {
  { "NONE",     .value = LEDS_UPDATE_NONE     },
  { "STATIC",   .value = LEDS_UPDATE_STATIC   },
  { "TEST",     .value = LEDS_UPDATE_TEST     },
  { "SEQUENCE", .value = LEDS_UPDATE_SEQUENCE },
  { "ARTNET",   .value = LEDS_UPDATE_ARTNET   },
  { "CMD",      .value = LEDS_UPDATE_CMD      },
  { "HTTP",     .value = LEDS_UPDATE_HTTP     },
  {}
};

static struct stats_timer get_leds_task_timer(struct leds_state *state)
{
  const struct leds_stats *stats = &leds_stats[state->index];

  return stats->loop;
}

static struct stats_timer get_leds_interface_timer(struct leds_state *state)
{
  enum leds_interface li = leds_interface(state->leds);
  struct i2s_out_stats i2s_out_stats;

  switch (li) {
    case LEDS_INTERFACE_I2S0:
    case LEDS_INTERFACE_I2S1:
      i2s_out_stats = get_leds_i2s_out_stats(leds_interface_i2s_port(li));

      return i2s_out_stats.out_timer;
    
    default:
      return (struct stats_timer) {};
  }
}

// basic moving average
static void update_stats_timer_metrics(struct stats_timer *baseline, const struct stats_timer *timer, struct stats_timer_metrics *avg)
{
  struct stats_timer_metrics metrics = stats_timer_diff_metrics(baseline, timer);

  *avg = stats_timer_metrics_average(avg, &metrics);

  *baseline = *timer;
}

void update_leds_status(struct leds_state *state)
{
  struct stats_timer task_timer = get_leds_task_timer(state);
  struct stats_timer interface_timer = get_leds_interface_timer(state);

  update_stats_timer_metrics(&state->status_timers.task, &task_timer, &state->status_timer_metrics.task);
  update_stats_timer_metrics(&state->status_timers.interface, &interface_timer, &state->status_timer_metrics.interface);
}

void get_leds_status(struct leds_state *state, struct leds_status *status)
{
  *status = (struct leds_status) {};

  // TODO: timer interval?
  update_leds_status(state);

  status->tick = xTaskGetTickCount();
  status->update_tick = state->update_tick;
  status->update_state = state->update_state;
  status->active = leds_is_active(state->leds);

  if ((status->test = !!state->test)) {
    status->test_mode = state->test->mode;
  }

  if ((status->artnet = !!state->artnet)) {
    status->artnet_dmx_tick = state->artnet->dmx_tick;
  }

  // limits
  status->limit_groups_count = LEDS_LIMIT_GROUPS_MAX;

  leds_get_limit_total_status(state->leds, &status->limit_total_status);
  leds_get_limit_groups_status(state->leds, status->limit_groups_status, &status->limit_groups_count);

  // metrics
  status->metrics = state->status_timer_metrics;
}
