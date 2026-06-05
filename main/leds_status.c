#include "leds_status.h"
#include "leds_state.h"
#include "leds_test.h"
#include "leds_artnet.h"
#include "leds.h"

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
}
