#include "leds.h"
#include "leds_artnet.h"
#include "leds_config.h"
#include "leds_state.h"
#include "leds_stats.h"
#include "artnet_state.h"
#include "tasks.h"

#include <artnet.h>
#include <logging.h>
#include <leds.h>

struct leds_artnet_test {
  enum leds_test_mode mode;

  TickType_t frame_tick;
  unsigned frame;
};

static bool leds_artnet_test_active(struct leds_artnet_test *test)
{
  return !!test->frame_tick;
}

/* Return number of ticks to wait for next test frame, or portMAX_DELAY if not in test mode */
static TickType_t leds_artnet_wait_ticks(struct leds_artnet_test *test, const struct leds_config *config)
{
  if (test->frame_tick) {
    TickType_t tick = xTaskGetTickCount();

    if (test->frame_tick > tick) {
      // time next test frame
      return test->frame_tick - tick;
    } else {
      // catchup
      return 0;
    }
  } else if (config->artnet_dmx_timeout) {
    // use loss-of-signal timeout
    return config->artnet_dmx_timeout / portTICK_PERIOD_MS;
  } else {
    // not in test mode
    return portMAX_DELAY;
  }
}

static void leds_artnet_test_start(struct leds_artnet_test *test)
{
  LOG_INFO("test start");

  test->mode = 0;
  test->frame_tick = xTaskGetTickCount();
  test->frame = 0;
}

static void leds_artnet_test_end(struct leds_artnet_test *test)
{
  LOG_INFO("test end");

  test->mode = TEST_MODE_END;
  test->frame_tick = 0;
  test->frame = 0;
}

static void leds_artnet_test_reset(struct leds_artnet_test *test)
{
  LOG_INFO("test reset");

  test->mode = 0;
  test->frame_tick = 0;
  test->frame = 0;
}

static void leds_artnet_test_frame(struct leds_state *state, struct leds_artnet_test *test)
{
  int frame_ticks;

  if (test->frame == 0) {
    LOG_INFO("test mode=%d", test->mode);
  }

  if ((frame_ticks = leds_set_test(state->leds, test->mode, test->frame)) < 0) {
    LOG_ERROR("leds_set_test");
    test->mode = 0;
    test->frame = 0;
  } else if (frame_ticks == 0) {
    test->mode++;
    test->frame = 0;
  } else {
    // tick for next frame
    test->frame++;
    test->frame_tick += frame_ticks;
  }

  // end of test cycle?
  if (test->mode > TEST_MODE_END) {
    leds_artnet_test_reset(test);
  }
}

static void leds_artnet_set(struct leds_state *state, unsigned index, struct artnet_dmx *dmx)
{
  // handle DMX address offset
  uint8_t *data = state->artnet.dmx->data;
  size_t len = state->artnet.dmx->len;

  if (state->config->artnet_dmx_addr) {
    unsigned addr = state->config->artnet_dmx_addr - 1;

    if (addr > len) {
      LOG_DEBUG("short universe len=%d for artnet_dmx_addr=(%d + 1)", len, addr);
      return;
    }

    data += addr;
    len -= addr;
  }

  // set LEDs from artnet data using configured byte format
  struct leds_format_params params = {
    .count = state->config->artnet_dmx_leds,
    .offset = index * state->config->artnet_dmx_leds,
    .segment = state->config->artnet_leds_segment,
  };

  leds_set_format(state->leds, state->config->artnet_leds_format, data, len, params);
}

static EventBits_t leds_artnet_wait(struct leds_state *state, struct leds_artnet_test *test_state)
{
  TickType_t wait_ticks = leds_artnet_wait_ticks(test_state, state->config);
  const bool clear_on_exit = true;
  const bool wait_for_all_bits = false;

  LOG_DEBUG("leds%d: notify wait ticks=%d", state->index + 1, wait_ticks);

  return xEventGroupWaitBits(state->artnet.event_group, ARTNET_OUTPUT_EVENT_INDEX_BITS | ARTNET_OUTPUT_EVENT_FLAG_BITS, clear_on_exit, wait_for_all_bits, wait_ticks);
}

static void leds_artnet_main(void *ctx)
{
  struct leds_state *state = ctx;
  struct leds_artnet_test test_state = {};

  for(struct stats_timer_sample loop_sample;; stats_timer_stop(&leds_stats_artnet_loop, &loop_sample)) {
    EventBits_t event_bits = leds_artnet_wait(state, &test_state);

    LOG_DEBUG("leds%d: notify index=%04x: sync=%d test=%d", state->index + 1,
      (event_bits & ARTNET_OUTPUT_EVENT_INDEX_BITS),
      !!(event_bits & (1 << ARTNET_OUTPUT_EVENT_SYNC_BIT)),
      !!(event_bits & (1 << LEDS_ARTNET_EVENT_TEST_BIT))
    );

    loop_sample = stats_timer_start(&leds_stats_artnet_loop);

    // start/stop test mode
    bool unsync = false, sync = false, test = false, clear = false;

    if (event_bits & (1 << ARTNET_OUTPUT_EVENT_SYNC_BIT)) {
      sync = true;
    }
    if (event_bits & (1 << LEDS_ARTNET_EVENT_TEST_BIT)) {
      leds_artnet_test_start(&test_state);
    }

    if (leds_artnet_test_active(&test_state)) {
      if ((event_bits & ARTNET_OUTPUT_EVENT_INDEX_BITS) || (event_bits & (1 << ARTNET_OUTPUT_EVENT_SYNC_BIT))) {
        // have output data, end test
        leds_artnet_test_end(&test_state);
      }

      WITH_STATS_TIMER(&leds_stats_artnet_test) {
        leds_artnet_test_frame(state, &test_state);
      }

      test = true;
    } else if (state->config->artnet_dmx_timeout && !event_bits) {
      LOG_DEBUG("leds%d: timeout", state->index + 1);

      stats_counter_increment(&leds_stats_artnet_timeout);

      clear = true;
    }

    // set output from artnet universe
    for (uint8_t index = 0; index < state->artnet.universe_count; index++) {
      if (!(event_bits & (1 << index))) {
        continue;
      }

      if (artnet_output_read(state->artnet.outputs[index], state->artnet.dmx, 0)) {
        LOG_WARN("leds%d: artnet_output[%d] empty", state->index + 1, index);
        continue;
      }

      WITH_STATS_TIMER(&leds_stats_artnet_set) {
        leds_artnet_set(state, index, state->artnet.dmx);
      }

      if (!state->artnet.dmx->sync_mode) {
        // at least one DMX update is in non-synchronous mode, so force update
        unsync = true;
      }
    }

    // tx output if required
    if (unsync || sync || test) {
      WITH_STATS_TIMER(&leds_stats_artnet_update) {
        if (update_leds(state)) {
          LOG_WARN("leds%d: update_leds", state->index + 1);
          continue;
        }
      }
    }

    if (clear) {
      WITH_STATS_TIMER(&leds_stats_artnet_update) {
        if (clear_leds(state)) {
          LOG_WARN("leds%d: clear_leds", state->index + 1);
          continue;
        }
      }
    }
  }
}

int init_leds_artnet(struct leds_state *state, int index, const struct leds_config *config)
{
  LOG_INFO("leds%d: universe start=%u count=%u step=%u dmx addr=%u leds=%u leds format=%s segment=%u", index + 1,
    config->artnet_universe_start,
    config->artnet_universe_count,
    config->artnet_universe_step,
    config->artnet_dmx_addr,
    config->artnet_dmx_leds,
    config_enum_to_string(leds_format_enum, config->artnet_leds_format),
    config->artnet_leds_segment
  );

  if (config->artnet_universe_count) {
    state->artnet.universe_count = config->artnet_universe_count;
  } else {
    state->artnet.universe_count = 1;
  }

  if (!(state->artnet.dmx = calloc(1, sizeof(*state->artnet.dmx)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(state->artnet.outputs = calloc(state->artnet.universe_count, sizeof(*state->artnet.outputs)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(state->artnet.event_group = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  return 0;
}

int start_leds_artnet(struct leds_state *state, const struct leds_config *config)
{
  struct task_options task_options = {
    .main       = leds_artnet_main,
    .name_fmt   = LEDS_ARTNET_TASK_NAME_FMT,
    .stack_size = LEDS_ARTNET_TASK_STACK,
    .arg        = state,
    .priority   = LEDS_ARTNET_TASK_PRIORITY,
    .handle     = &state->artnet.task,
    .affinity   = LEDS_ARTNET_TASK_AFFINITY,
  };

  if (start_taskf(task_options, state->index + 1)) {
    LOG_ERROR("start_taskf");
    return -1;
  } else {
    LOG_INFO("leds%d: start artnet task=%p", state->index + 1, state->artnet.task);
  }

  for (uint8_t i = 0; i < state->artnet.universe_count; i++) {
    struct artnet_output_options options = {
      .port = (enum artnet_port) (state->index), // use ledsX index as output port
      .index = i,
      .address = config->artnet_universe_start + i * config->artnet_universe_step, // net/subnet is set by add_artnet_output()
      .event_group = state->artnet.event_group,
    };

    snprintf(options.name, sizeof(options.name), "leds%u", state->index + 1);

    if (add_artnet_output(&state->artnet.outputs[i], options)) {
      LOG_ERROR("add_artnet_output");
      return -1;
    }
  }

  return 0;
}
