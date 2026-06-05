#include "leds_status.h"
#include "leds_state.h"
#include "leds_stats.h"
#include "leds_test.h"
#include "leds_artnet.h"
#include "leds.h"

void get_leds_stats_status(struct leds_state *state, struct leds_status *status)
{
  const struct leds_stats *stats = &leds_stats[state->index];

  status->task_rate = stats_timer_average_rate(&stats->loop);
  status->task_util = stats_timer_utilization(&stats->loop);
}

void get_leds_interface_status(struct leds_state *state, struct leds_status *status)
{
  enum leds_interface li = leds_interface(state->leds);
  struct i2s_out_stats i2s_out_stats;

  switch (li) {
    case LEDS_INTERFACE_I2S0:
    case LEDS_INTERFACE_I2S1:
      i2s_out_stats = get_leds_i2s_out_stats(leds_interface_i2s_port(li));

      status->interface_rate = stats_timer_average_rate(&i2s_out_stats.out_timer);
      status->interface_util = stats_timer_utilization(&i2s_out_stats.out_timer);

      break;
    
    default:
      break;
  }
}

void get_leds_status(struct leds_state *state, struct leds_status *status)
{
  *status = (struct leds_status) {};

  status->tick = xTaskGetTickCount();
  status->update_tick = state->update_tick;
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

  // stats
  get_leds_stats_status(state, status);
  get_leds_interface_status(state, status);
}
