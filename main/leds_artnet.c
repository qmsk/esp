#include "leds.h"
#include "leds_artnet.h"
#include "leds_config.h"
#include "leds_state.h"
#include "leds_stats.h"
#include "leds_task.h"
#include "leds_test.h"
#include "artnet_state.h"

#include <artnet.h>
#include <logging.h>
#include <leds.h>

#define SYNC_BIT(index) (1 << (index))
#define SYNC_BITS_MASK(count) ((1 << (count)) - 1)

unsigned config_leds_artnet_universe_leds_count(const struct leds_config *config)
{
  unsigned universe_leds_count = config->artnet_dmx_leds;

  if (!universe_leds_count) {
    universe_leds_count = leds_format_count(ARTNET_DMX_SIZE, config->artnet_leds_format, config->artnet_leds_group);
  }

  return universe_leds_count;
}

unsigned config_leds_artnet_universe_count(const struct leds_config *config)
{
  unsigned universe_leds_count = config_leds_artnet_universe_leds_count(config);
  unsigned universe_count = config->artnet_universe_count;
  unsigned leds_count = config->count;

  if (config->artnet_leds_segment) {
    // artnet pixel count
    leds_count = leds_count / config->artnet_leds_segment;
  }

  if (!universe_leds_count) {
    // invalid
    return 0;
  }

  if (!universe_count) {
    // how many universes do we need to fit all of the frames?
    universe_count = leds_count / universe_leds_count;

    if (leds_count % universe_leds_count) {
      universe_count += 1;
    }
  }

  return universe_count;
}

unsigned count_leds_artnet_outputs()
{
  unsigned count = 0;

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    const struct leds_config *config = &leds_configs[i];

    if (!config->enabled) {
      continue;
    }

    count += config_leds_artnet_universe_count(config);
  }

  return count;
}

static bool leds_artnet_sync_set(struct leds_state *state, unsigned index)
{
  const struct leds_config *config = state->config;
  bool update = true;

  if (!(state->artnet->sync_bits & SYNC_BIT(index))) {
    // mark for sync as normal
    state->artnet->sync_bits |= SYNC_BIT(index);
  } else {
    LOG_DEBUG("missed index=%u", index);

    // mark for missed sync
    state->artnet->sync_missed |= SYNC_BIT(index);

    // delay update to next sync
    update = false;
  }

  if (config->artnet_sync_timeout) {
    // set on first update
    if (!state->artnet->sync_tick) {
      state->artnet->sync_tick = xTaskGetTickCount() + config->artnet_sync_timeout / portTICK_PERIOD_MS;
    }
  } else {
    state->artnet->sync_tick = 0;
  }

  return update;
}

static bool leds_artnet_sync_check(struct leds_state *state)
{
  const struct leds_config *config = state->config;
  struct leds_stats *stats = &leds_stats[state->index];

  if (!config->artnet_sync_timeout && state->artnet->sync_bits) {
    // any output set, free-running
    stats_counter_increment(&stats->sync_none);

    return true;
  }

  if (state->artnet->sync_bits == SYNC_BITS_MASK(state->artnet->universe_count)) {
    // all outputs set
    stats_counter_increment(&stats->sync_full);

    return true;
  }

  if (state->artnet->sync_missed) {
    // any outputs missed
    stats_counter_increment(&stats->sync_missed);

    return true;
  }

  if (state->artnet->sync_tick && xTaskGetTickCount() >= state->artnet->sync_tick) {
    // timed out waiting for remaining outputs
    stats_counter_increment(&stats->sync_timeout);

    return true;
  }

  // wait for remaining outputs update before sync
  return false;
}

static void leds_artnet_sync_reset(struct leds_state *state)
{
  const struct leds_config *config = state->config;

  state->artnet->sync_bits = 0;

  if (state->artnet->sync_missed && config->artnet_sync_timeout) {
    // prepare to sync any missed updates per this tick
    state->artnet->sync_tick = xTaskGetTickCount() + config->artnet_sync_timeout / portTICK_PERIOD_MS;
  } else {
    state->artnet->sync_tick = 0;
  }
}

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
  if (state->artnet->sync_missed) {
    // immediately
    return xTaskGetTickCount();
  }

  if (state->artnet->sync_tick) {
    return state->artnet->sync_tick;
  }

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

  // set LEDs from artnet data using configured byte format
  unsigned count = state->artnet->universe_leds_count;
  unsigned segment = config->artnet_leds_segment ? config->artnet_leds_segment : 1;

  struct leds_format_params params = {
    .count   = count,
    .index  = index * count * segment,
    .segment = segment,
    .group   = config->artnet_leds_group,
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
  if (event_bits & (1 << LEDS_EVENT_ARTNET_DMX_BIT)) {
    return true;
  }

  if (event_bits & (1 << LEDS_EVENT_ARTNET_SYNC_BIT)) {
    return true;
  }

  if (state->artnet->sync_missed) {
    return true;
  }

  if (state->artnet->sync_tick) {
    if (xTaskGetTickCount() >= state->artnet->sync_tick) {
      return true;
    }
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
  struct leds_stats *stats = &leds_stats[state->index];

  EventBits_t data_bits = xEventGroupClearBits(state->artnet->event_group, ARTNET_OUTPUT_EVENT_BITS);

  bool dmx = event_bits & (1 << LEDS_EVENT_ARTNET_DMX_BIT);
  bool sync = event_bits & (1 << LEDS_EVENT_ARTNET_SYNC_BIT);
  bool miss = false;

  bool update = false;
  bool timeout = false;

  if (state->test) {
    if (dmx || sync) {
      // clear any test mode output
      leds_test_clear(state);
    }
  }

  if (state->artnet->sync_missed) {
    LOG_DEBUG("event_bits=%08x + sync_missed=%08x", event_bits, state->artnet->sync_missed);

    // handle updates with missed sync
    data_bits |= state->artnet->sync_missed;
    miss = true;

    state->artnet->sync_missed = 0;
  }

  // wait until either artnet-sync or (non-sync) dmx to not trigger soft-sync on partial data in artnet sync mode
  if (dmx || sync || miss) {
    // set output from artnet universe
    for (unsigned index = 0; index < state->artnet->universe_count; index++) {
      if (!(data_bits & (1 << index))) {
        continue;
      }

      if (sync) {
        // skip soft-sync
      } else if (leds_artnet_sync_set(state, index)) {
        // normal soft-sync update
      } else if (miss) {
        // catch up to missed soft-sync update 
        LOG_DEBUG("hit index=%u", index);
      } else {
        LOG_DEBUG("skip index=%u", index);

        // update with previous value for missed sync, will be read for next update
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
    }
  }

  if (sync) {
    // hard art-net sync
    stats_counter_increment(&stats->artnet_sync);

    update = true;
  } else if (leds_artnet_sync_check(state)) {
    // soft sync
    update = true;
  }

  // timeouts
  if (dmx || sync) {
    state->artnet->dmx_tick = xTaskGetTickCount();
    leds_artnet_timeout_reset(state);
  } else if (state->artnet->timeout_tick) {
    if (xTaskGetTickCount() >= state->artnet->timeout_tick) {
      timeout = true;

      if (leds_artnet_timeout(state)) {
        LOG_WARN("leds_artnet_timeout");
      }

      // repeat
      leds_artnet_timeout_reset(state);
    }
  }

  if (update) {
    leds_artnet_sync_reset(state);
  }

  if (timeout) {
    return LEDS_ARTNET_UPDATE_TIMEOUT;
  } else if (update) {
    return LEDS_ARTNET_UPDATE;
  } else {
    return 0;
  }
}

int init_leds_artnet(struct leds_state *state, int index, const struct leds_config *config)
{
  LOG_INFO("leds%d: universe start=%u count=%u step=%u dmx addr=%u leds=%u leds format=%s segment=%u group=%u", index + 1,
    config->artnet_universe_start,
    config->artnet_universe_count,
    config->artnet_universe_step,
    config->artnet_dmx_addr,
    config->artnet_dmx_leds,
    config_enum_to_string(leds_format_enum, config->artnet_leds_format),
    config->artnet_leds_segment,
    config->artnet_leds_group
  );

  if (!(state->artnet = calloc(1, sizeof(*state->artnet)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(state->artnet->universe_leds_count = config_leds_artnet_universe_leds_count(config))) {
    LOG_ERROR("invalid universe_leds_count, artnet_leds_group is too large?");
    return -1;
  }

  state->artnet->universe_count = config_leds_artnet_universe_count(config);

  LOG_INFO("universe_count=%u universe_leds_count=%u",
    state->artnet->universe_count,
    state->artnet->universe_leds_count
  );

  if (!(state->artnet->outputs = calloc(state->artnet->universe_count, sizeof(*state->artnet->outputs)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(state->artnet->event_group = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  leds_artnet_timeout_reset(state);

  return 0;
}

int start_leds_artnet(struct leds_state *state, const struct leds_config *config)
{
  if (!artnet) {
    LOG_ERROR("artnet disabled");
    return -1;
  }

  for (unsigned i = 0; i < state->artnet->universe_count; i++) {
    unsigned artnet_universe = config->artnet_universe_start + i * config->artnet_universe_step;

    struct artnet_output_options options = {
      .address = artnet_address(config->artnet_net, config->artnet_subnet, artnet_universe),

      // wake up task on each art-net sync/update
      .event_group = state->event_group,
      .dmx_event_bit = (1 << LEDS_EVENT_ARTNET_DMX_BIT),
      .sync_event_bit = (1 << LEDS_EVENT_ARTNET_SYNC_BIT),

      // mark updated outputs via a separate event group
      .output_events = state->artnet->event_group,
      .output_event_bit = (1 << i),
    };

    snprintf(options.name, sizeof(options.name), "leds%u", state->index + 1);
  
    LOG_INFO("name=%s address=%04x", options.name, options.address);

    if (artnet_add_output(artnet, &state->artnet->outputs[i], options)) {
      LOG_ERROR("artnet_add_output");
      return -1;
    }
  }

  return 0;
}
