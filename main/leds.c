#include "leds.h"
#include "leds_artnet.h"
#include "leds_state.h"
#include "leds_config.h"
#include "user.h"

#include <leds.h>

#include <logging.h>

struct leds_state leds_states[LEDS_COUNT] = {};

int init_leds()
{
  int err;

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    struct leds_state *state = &leds_states[i];
    const struct leds_config *config = &leds_configs[i];

    state->index = i;
    state->config = config;

    if (!config->enabled) {
      continue;
    }

    if ((err = config_leds(state, i, config))) {
      LOG_ERROR("leds%d: config_leds", i+1);
      return err;
    }

    if (config->test_enabled) {
      if ((err = test_leds(state))) {
        LOG_ERROR("leds%d: test_leds", i + 1);
        return err;
      }
    }

    if (config->artnet_enabled) {
      if ((err = init_leds_artnet(state, i, config))) {
        LOG_ERROR("leds%d: init_leds_artnet", i + 1);
        return err;
      }
    }
  }

  return 0;
}

int start_leds()
{
  // TODO:
  return 0;
}

int update_leds(struct leds_state *state)
{
  int err;

  user_activity(USER_ACTIVITY_LEDS);

  if ((err = leds_tx(state->leds))) {
    LOG_ERROR("leds_tx");
    return err;
  }

  return 0;
}

int test_leds_mode(struct leds_state *state, enum leds_test_mode mode)
{
  int err;

  LOG_INFO("mode=%d", mode);

  user_activity(USER_ACTIVITY_LEDS);

  // animate
  TickType_t tick = xTaskGetTickCount();
  int ticks;

  for (unsigned frame = 0; ; frame++) {
    if ((ticks = leds_set_test(state->leds, mode, frame)) < 0) {
      LOG_ERROR("leds%d: leds_set_test(%d, %u)", state->index + 1, mode, frame);

      return -1;
    }

    if ((err = leds_tx(state->leds))) {
      LOG_ERROR("leds%d: leds_tx", state->index + 1);
      return err;
    }

    if (ticks) {
      vTaskDelayUntil(&tick, ticks);
    } else {
      break;
    }
  }

  return 0;
}

int test_leds(struct leds_state *state)
{
  int err;

  for (enum leds_test_mode mode = 0; mode <= TEST_MODE_END; mode++) {
    if ((err = test_leds_mode(state, mode))) {
      LOG_ERROR("leds%d: test_leds", state->index + 1);
      return err;
    }
  }

  return 0;
}
