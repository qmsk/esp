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

    if (set_leds_test_next(state)) {
      LOG_ERROR("set_leds_test_next");
    }
  }
}

void auto_leds_test()
{
  for (int i = 0; i < LEDS_COUNT; i++) {
    struct leds_state *state = &leds_states[i];

    if (!state->test) {
      continue;
    }

    if (set_leds_test_auto(state)) {
      LOG_ERROR("set_leds_test_auto");
    }
  }
}

void reset_leds_test()
{
  for (int i = 0; i < LEDS_COUNT; i++) {
    struct leds_state *state = &leds_states[i];

    if (!state->test) {
      continue;
    }

    if (clear_leds_test(state)) {
      LOG_ERROR("clear_leds_test");
    }
  }
}

static void leds_test_next_mode(struct leds_state *state)
{
  if (state->test->mode >= TEST_MODE_COUNT) {
    // cycle
    state->test->mode = 1;
  } else {
    // next;
    state->test->mode++;
  }
  
  state->test->frame = 0;
  state->test->frame_tick = 0;
}

int set_leds_test(struct leds_state *state, enum leds_test_mode mode, bool auto_mode)
{
  if (!state->test) {
    LOG_ERROR("disabled");
    return -1;
  }

  state->test->mode = mode;
  state->test->frame = 0;
  state->test->auto_mode = auto_mode;

  LOG_DEBUG("mode=%d auto_mode=%d", state->test->mode, state->test->auto_mode);

  notify_leds_task(state, 1 << LEDS_EVENT_TEST_BIT);

  return 0;
}

int set_leds_test_auto(struct leds_state *state)
{
  if (!state->test) {
    LOG_ERROR("disabled");
    return -1;
  }

  state->test->auto_mode = true;

  if (!state->test->mode || !state->test->frame_tick) {
    leds_test_next_mode(state);
  }

  LOG_DEBUG("mode=%d auto_mode=%d", state->test->mode, state->test->auto_mode);

  notify_leds_task(state, 1 << LEDS_EVENT_TEST_BIT);

  return 0;
}

int set_leds_test_next(struct leds_state *state)
{
  if (!state->test) {
    LOG_ERROR("disabled");
    return -1;
  }

  leds_test_next_mode(state);

  LOG_DEBUG("mode=%d auto_mode=%d", state->test->mode, state->test->auto_mode);

  notify_leds_task(state, 1 << LEDS_EVENT_TEST_BIT);

  return 0;
}

int clear_leds_test(struct leds_state *state)
{
  if (!state->test) {
    LOG_ERROR("disabled");
    return -1;
  }

  // will be reset to 0 by leds_test_update()
  state->test->mode = TEST_MODE_NONE;
  state->test->auto_mode = false;

  LOG_DEBUG("mode=%d auto_mode=%d", state->test->mode, state->test->auto_mode);

  notify_leds_task(state, 1 << LEDS_EVENT_TEST_BIT);

  return 0;
}

void leds_test_clear(struct leds_state *state)
{
  state->test->mode = 0;
  state->test->frame = 0;
  state->test->frame_tick = 0;
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
  if (state->test->mode) {
    return true;
  }

  if (bits & (1 << LEDS_EVENT_TEST_BIT)) {
    return true;
  }

  return false;
}

int leds_test_update(struct leds_state *state, EventBits_t bits)
{
  TickType_t tick = xTaskGetTickCount();
  int frame_ticks;

  if (!state->test->mode) {
    LOG_DEBUG("clear mode=%d auto=%d frame=%d frame_tick=%d", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick);

    if (leds_clear_all(state->leds)) {
      LOG_WARN("leds_clear_all");
    }

    leds_test_clear(state);

    return 1;

  } else if (state->test->frame == 0) {
    state->test->frame_tick = tick;

    LOG_DEBUG("start mode=%d auto=%d frame=%d frame_tick=%d", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick);

  } else if (!state->test->frame_tick) {
    LOG_DEBUG("idle mode=%d auto=%d frame=%d frame_tick=%d", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick);

    return 0;

  } else if (state->test->frame_tick < tick) {
    LOG_DEBUG("wait mode=%d auto=%d frame=%d frame_tick=%d", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick);

    return 0;
  }

  // TODO: disable sequence?

  if ((frame_ticks = leds_set_test(state->leds, state->test->mode, state->test->frame)) < 0) {
    LOG_ERROR("leds_set_test");
    return -1;

  } else if (frame_ticks) {
    // tick for next frame
    LOG_DEBUG("mode=%d auto=%d frame=%d frame_tick=%d -> next frame_ticks=%d", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick, frame_ticks);

    state->test->frame++;
    state->test->frame_tick += frame_ticks;

  } else if (state->test->auto_mode) {
    // advance to next mode
    LOG_DEBUG("mode=%d auto=%d frame=%d frame_tick=%d -> next mode", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick);

    leds_test_next_mode(state);

  } else {
    // stop
    LOG_DEBUG("mode=%d auto=%d frame=%d frame_tick=%d -> stop", state->test->mode, state->test->auto_mode, state->test->frame, state->test->frame_tick);

    state->test->frame_tick = 0;
  }

  if (state->artnet) {
    leds_artnet_timeout_clear(state);
  }

  return 1;
}
