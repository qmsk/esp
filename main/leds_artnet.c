#include "leds.h"
#include "leds_artnet.h"
#include "leds_config.h"
#include "leds_state.h"
#include "leds_stats.h"
#include "leds_test.h"
#include "artnet_state.h"

#include <artnet.h>
#include <logging.h>
#include <leds.h>

void leds_artnet_timeout_reset(struct leds_state *state)
{
  const struct leds_config *config = state->config;

  if (config->artnet_dmx_timeout) {
    state->artnet->timeout_tick = xTaskGetTickCount() + config->artnet_dmx_timeout / portTICK_PERIOD_MS;
  } else {
    state->artnet->timeout_tick = 0;
  }
}

TickType_t leds_artnet_wait(struct leds_state *state)
{
  if (state->artnet->timeout_tick && !leds_test_active(state, 0)) {
    // use loss-of-signal timeout
    return state->artnet->timeout_tick;
  }

  return 0;
}

static int leds_artnet_set(struct leds_state *state, unsigned index, struct artnet_dmx *dmx)
{
  const struct leds_config *config = state->config;
  int err;

  // handle DMX address offset
  uint8_t *data = state->artnet->dmx.data;
  size_t len = state->artnet->dmx.len;

  if (config->artnet_dmx_addr) {
    unsigned addr = config->artnet_dmx_addr - 1;

    if (addr > len) {
      LOG_DEBUG("short universe len=%d for artnet_dmx_addr=(%d + 1)", len, addr);
      return -1;
    }

    data += addr;
    len -= addr;
  }

  // count of LEds per universe
  unsigned count = config->artnet_dmx_leds;

  if (count == 0) {
    count = leds_get_format_count(config->artnet_leds_format, ARTNET_DMX_SIZE);
  }

  // set LEDs from artnet data using configured byte format
  struct leds_format_params params = {
    .count = count,
    .offset = index * count,
    .segment = config->artnet_leds_segment,
  };

  if ((err = leds_set_format(state->leds, config->artnet_leds_format, data, len, params))) {
    LOG_WARN("leds_set_format");
    return err;
  }

  return 0;
}

static int leds_artnet_timeout(struct leds_state *state)
{
  struct leds_stats *stats = &leds_stats[state->index];
  int err;

  LOG_INFO("leds%d: timeout", state->index + 1);

  stats_counter_increment(&stats->artnet_timeout);

  // TODO: flash user alert?

  if ((err = leds_clear_all(state->leds))) {
    LOG_WARN("leds_clear_all");
    return err;
  }

  return 0;
}

bool leds_artnet_active(struct leds_state *state, EventBits_t event_bits)
{
  if (!state->artnet) {
    return false;
  }

  if ((event_bits & ARTNET_OUTPUT_EVENT_INDEX_BITS)) {
    return true;
  }

  if (event_bits & (1 << ARTNET_OUTPUT_EVENT_SYNC_BIT)) {
    return true;
  }

  if (state->artnet->timeout_tick && !leds_test_active(state, 0)) {
    if (xTaskGetTickCount() >= state->artnet->timeout_tick) {
      return true;
    }
  }

  return false;
}

int leds_artnet_update(struct leds_state *state, EventBits_t event_bits)
{
  const struct leds_config *config = state->config;

  bool data = event_bits & ARTNET_OUTPUT_EVENT_INDEX_BITS;
  bool sync = event_bits & (1 << ARTNET_OUTPUT_EVENT_SYNC_BIT);
  bool unsync = false;
  bool timeout = false;

  if (state->test) {
    if (data || sync) {
      // clear any test mode output
      leds_test_clear(state);
    }
  }

  if (data) {
    // set output from artnet universe
    for (uint8_t index = 0; index < state->artnet->universe_count; index++) {
      if (!(event_bits & (1 << index))) {
        continue;
      }

      if (artnet_output_read(state->artnet->outputs[index], &state->artnet->dmx, 0)) {
        // this can race under normal conditions, we have already handled the output
        LOG_DEBUG("leds%d: artnet_output[%d] empty", state->index + 1, index);
        continue;
      }

      if (leds_artnet_set(state, index, &state->artnet->dmx)) {
        LOG_WARN("leds%d: leds_artnet_set", state->index + 1);
        continue;
      }

      if (!state->artnet->dmx.sync_mode) {
        // at least one DMX update is in non-synchronous mode, so force update
        unsync = true;
      }
    }
  }

  if (data || sync) {
    leds_artnet_timeout_reset(state);
  } else if (config->artnet_dmx_timeout) {
    if (xTaskGetTickCount() >= state->artnet->timeout_tick) {
      if (leds_artnet_timeout(state)) {
        LOG_WARN("leds_artnet_timeout");
      }

      timeout = true;

      leds_artnet_timeout_reset(state);
    }
  }

  return unsync || sync || timeout;
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

  if (!(state->artnet = calloc(1, sizeof(*state->artnet)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (config->artnet_universe_count) {
    state->artnet->universe_count = config->artnet_universe_count;
  } else {
    state->artnet->universe_count = 1;
  }

  if (!(state->artnet->outputs = calloc(state->artnet->universe_count, sizeof(*state->artnet->outputs)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  leds_artnet_timeout_reset(state);

  return 0;
}

int start_leds_artnet(struct leds_state *state, const struct leds_config *config)
{
  for (uint8_t i = 0; i < state->artnet->universe_count; i++) {
    struct artnet_output_options options = {
      .port = (enum artnet_port) (state->index), // use ledsX index as output port
      .index = i,
      .address = config->artnet_universe_start + i * config->artnet_universe_step, // net/subnet is set by add_artnet_output()
      .event_group = state->event_group,
    };

    snprintf(options.name, sizeof(options.name), "leds%u", state->index + 1);

    if (add_artnet_output(&state->artnet->outputs[i], options)) {
      LOG_ERROR("add_artnet_output");
      return -1;
    }
  }

  return 0;
}
