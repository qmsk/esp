#include "leds_artnet.h"
#include "leds_task.h"
#include "leds_test.h"

#include <logging.h>

int init_leds_test(struct leds_state *state, const struct leds_config *config)
{
  if (!(state->test = calloc(1, sizeof(*state->test)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  state->test->mode = config->test_mode;
  state->test->auto_mode = config->test_auto;

  if (state->test->mode || state->test->auto_mode) {
    LOG_INFO("mode=%d auto_mode=%d", state->test->mode, state->test->auto_mode);

    // required to start initial tick
    notify_leds_task(state, 1 << LEDS_EVENT_TEST_BIT);
  }

  return 0;
}

void trigger_leds_test()
{
  for (int i = 0; i < LEDS_COUNT; i++) {
    struct leds_state *state = &leds_states[i];

    if (!state->test) {
      continue;
    }

    if (state->test->mode >= TEST_MODE_BLACK) {
      // cycle
      state->test->mode = 0;
    }

    state->test->mode++;
    state->test->frame = 0;

    LOG_DEBUG("mode=%d auto_mode=%d", state->test->mode, state->test->auto_mode);

    notify_leds_task(state, 1 << LEDS_EVENT_TEST_BIT);
  }
}

void auto_leds_test()
{
  for (int i = 0; i < LEDS_COUNT; i++) {
    struct leds_state *state = &leds_states[i];

    if (!state->test) {
      continue;
    }

    if (!state->test->mode) {
      state->test->mode++;
    }

    state->test->auto_mode = true;

    LOG_DEBUG("mode=%d auto_mode=%d", state->test->mode, state->test->auto_mode);

    notify_leds_task(state, 1 << LEDS_EVENT_TEST_BIT);
  }
}

void reset_leds_test()
{
  for (int i = 0; i < LEDS_COUNT; i++) {
    struct leds_state *state = &leds_states[i];

    if (!state->test) {
      continue;
    }

    // will be reset to 0 by leds_test_update()
    state->test->mode = TEST_MODE_BLACK;
    state->test->auto_mode = false;

    LOG_DEBUG("mode=%d auto_mode=%d", state->test->mode, state->test->auto_mode);

    notify_leds_task(state, 1 << LEDS_EVENT_TEST_BIT);
  }
}

static void leds_test_reset(struct leds_state *state)
{
  state->test->mode = 0;
  state->test->frame = 0;
  state->test->frame_tick = 0;
}

void leds_test_clear(struct leds_state *state)
{
  if (!state->test->mode) {
    return;
  }

  // incoming artnet data overrides test mode
  if (leds_clear_all(state->leds)) {
    LOG_WARN("leds_clear_all");
  }

  leds_test_reset(state);
}

TickType_t leds_test_wait(struct leds_state *state)
{
  if (!state->test->mode) {
    return 0;
  }

  // tick for next test frame
  return state->test->frame_tick;
}


bool leds_test_active(struct leds_state *state, EventBits_t bits)
{
  if (state->test->mode && state->test->frame_tick) {
    return true;
  }

  if (bits & (1 << LEDS_EVENT_TEST_BIT)) {
    return true;
  }

  return false;
}

int leds_test_update(struct leds_state *state, EventBits_t bits)
{
  int frame_ticks;

  if (state->test->frame == 0) {
    state->test->frame_tick = xTaskGetTickCount();

    LOG_INFO("test mode=%d", state->test->mode);
  }

  if ((frame_ticks = leds_set_test(state->leds, state->test->mode, state->test->frame)) < 0) {
    LOG_ERROR("leds_set_test");
    return -1;
  } else if (frame_ticks) {
    // tick for next frame
    LOG_DEBUG("mode=%d auto=%d frame=%d frame_tick=%d -> wait frame_ticks=%d", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick, frame_ticks);

    state->test->frame++;
    state->test->frame_tick += frame_ticks;
  } else if (state->test->mode == TEST_MODE_BLACK) {
    // end
    LOG_DEBUG("mode=%d auto=%d frame=%d frame_tick=%d -> reset", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick);

    leds_test_reset(state);
  } else if (state->test->auto_mode) {
    // advance to next mode
    LOG_DEBUG("mode=%d auto=%d frame=%d frame_tick=%d -> next mode", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick);

    state->test->mode++;
    state->test->frame = 0;
  } else {
    // pause
    LOG_DEBUG("mode=%d auto=%d frame=%d frame_tick=%d -> pause", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick);

    state->test->frame_tick = 0;
  }

  if (state->artnet) {
    leds_artnet_timeout_reset(state);
  }

  return 1;
}
