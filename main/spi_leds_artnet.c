#include "spi_leds.h"
#include "artnet.h"

#include <artnet.h>
#include <logging.h>
#include <spi_leds.h>

#define SPI_LEDS_ARTNET_TASK_NAME_FMT "spi-leds%d"
#define SPI_LEDS_ARTNET_TASK_STACK 1024
#define SPI_LEDS_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

struct spi_leds_artnet_test {
  enum spi_leds_test_mode mode;

  TickType_t frame_tick;
  unsigned frame;
};

static bool spi_leds_artnet_test_active(struct spi_leds_artnet_test *test)
{
  return !!test->frame_tick;
}

/* Return number of ticks to wait for next test frame, or portMAX_DELAY if not in test mode */
static TickType_t spi_leds_artnet_wait_ticks(struct spi_leds_artnet_test *test)
{
  if (!test->frame_tick) {
    // not in test mode
    return portMAX_DELAY;
  } else {
    TickType_t tick = xTaskGetTickCount();

    if (test->frame_tick > tick) {
      // time next test frame
      return test->frame_tick - tick;
    } else {
      // catchup
      return 0;
    }
  }
}

static void spi_leds_artnet_test_start(struct spi_leds_artnet_test *test)
{
  LOG_INFO("test start");

  test->mode = 0;
  test->frame_tick = xTaskGetTickCount();
  test->frame = 0;
}

static void spi_leds_artnet_test_stop(struct spi_leds_artnet_test *test)
{
  LOG_INFO("test stop");

  test->mode = 0;
  test->frame_tick = 0;
  test->frame = 0;
}

static void spi_leds_artnet_test_frame(struct spi_leds_state *state, struct spi_leds_artnet_test *test)
{
  int frame_ticks;

  if (test->frame == 0) {
    LOG_INFO("test mode=%d", test->mode);
  }

  if ((frame_ticks = spi_leds_set_test(state->spi_leds, test->mode, test->frame)) < 0) {
    LOG_ERROR("spi_leds_set_test");
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
  if (test->mode >= TEST_MODE_MAX) {
    spi_leds_artnet_test_stop(test);
  }
}

static void spi_leds_artnet_out(struct spi_leds_state *state, unsigned index, struct artnet_dmx *dmx)
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
  struct spi_leds_format_params params = {
    .count = state->config->artnet_universe_leds,
    .offset = index * state->config->artnet_universe_leds,
    .segment = state->config->artnet_leds_segment,
  };

  spi_leds_set_format(state->spi_leds, state->config->artnet_mode, data, len, params);
}

static void spi_leds_artnet_main(void *ctx)
{
  struct spi_leds_state *state = ctx;
  struct spi_leds_artnet_test test_state = {};

  for (;;) {
    uint32_t notify_bits;
    bool unsync = false, sync = false, test = false;
    TickType_t wait_ticks = spi_leds_artnet_wait_ticks(&test_state);

    LOG_DEBUG("notify wait ticks=%d", wait_ticks);

    // wait for output/sync, or next test frame
    xTaskNotifyWait(0, ARTNET_OUTPUT_TASK_INDEX_BITS | ARTNET_OUTPUT_TASK_FLAG_BITS, &notify_bits, wait_ticks);

    LOG_DEBUG("notify index=%04x: sync=%d test=%d",
      (notify_bits & ARTNET_OUTPUT_TASK_INDEX_BITS),
      !!(notify_bits & ARTNET_OUTPUT_TASK_SYNC_BIT),
      !!(notify_bits & ARTNET_OUTPUT_TASK_TEST_BIT)
    );

    // start/stop test mode
    if (notify_bits & ARTNET_OUTPUT_TASK_INDEX_BITS) {
      // have output data, reset test
      spi_leds_artnet_test_stop(&test_state);
    } else if (notify_bits & ARTNET_OUTPUT_TASK_SYNC_BIT) {
      sync = true;
    } else if (notify_bits & ARTNET_OUTPUT_TASK_TEST_BIT) {
      spi_leds_artnet_test_start(&test_state);
      spi_leds_artnet_test_frame(state, &test_state);

      test = true;
    } else if (spi_leds_artnet_test_active(&test_state)) {
      spi_leds_artnet_test_frame(state, &test_state);

      test = true;
    }

    // set output from artnet universe
    for (uint8_t index = 0; index < state->artnet.universe_count; index++) {
      if (!(notify_bits & 1 << index)) {
        continue;
      }

      if (!xQueueReceive(state->artnet.queues[index], state->artnet.dmx, 0)) {
        LOG_WARN("xQueueReceive");
        continue;
      }

      spi_leds_artnet_out(state, index, state->artnet.dmx);

      if (!state->artnet.dmx->sync_mode) {
        // at least one DMX update is in non-synchronous mode, so force update
        unsync = true;
      }
    }

    // tx output if required
    if (unsync || sync || test) {
      if (update_spi_leds(state)) {
        LOG_WARN("update_spi_leds");
        continue;
      }
    }
  }
}

int init_spi_leds_artnet(struct spi_leds_state *state, unsigned index, const struct spi_leds_config *config)
{
  char task_name[configMAX_TASK_NAME_LEN];

  LOG_INFO("spi-leds%u: mode=%x universe start=%u count=%u step=%u leds=%u", index,
    config->artnet_mode,
    config->artnet_universe_start,
    config->artnet_universe_count,
    config->artnet_universe_step,
    config->artnet_universe_leds
  );

  if (!(state->artnet.dmx = calloc(1, sizeof(*state->artnet.dmx)))) {
    LOG_ERROR("calloc");
    return -1;
  }
  if (!(state->artnet.queues = calloc(config->artnet_universe_count, sizeof(*state->artnet.queues)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  state->artnet.universe_count = config->artnet_universe_count;

  for (uint8_t i = 0; i < config->artnet_universe_count; i++) {
    if (!(state->artnet.queues[i] = xQueueCreate(1, sizeof(struct artnet_dmx)))) {
      LOG_ERROR("xQueueCreate");
      return -1;
    }
  }

  // task
  snprintf(task_name, sizeof(task_name), SPI_LEDS_ARTNET_TASK_NAME_FMT, index);

  if (xTaskCreate(&spi_leds_artnet_main, task_name, SPI_LEDS_ARTNET_TASK_STACK, state, SPI_LEDS_ARTNET_TASK_PRIORITY, &state->artnet.task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("task=%p", state->artnet.task);
  }

  for (uint8_t i = 0; i < config->artnet_universe_count; i++) {
    struct artnet_output_options options = {
      .port = (enum artnet_output_port) (index), // use spi-ledsX index as output port
      .index = i,
      .address = config->artnet_universe_start + i * config->artnet_universe_step, // net/subnet is set by add_artnet_output()
      .task = state->artnet.task,
    };

    LOG_INFO("spi-leds%u: artnet output port=%d address=%04x index=%u", index, options.port, options.address, options.index);

    if (add_artnet_output(options, state->artnet.queues[i])) {
      LOG_ERROR("add_artnet_output");
      return -1;
    }
  }

  return 0;
}
