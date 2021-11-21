#include "spi_leds.h"
#include "artnet.h"

#include <artnet.h>
#include <logging.h>
#include <spi_leds.h>

#define SPI_LEDS_ARTNET_TASK_NAME_FMT "spi-leds%d"
#define SPI_LEDS_ARTNET_TASK_STACK 1024
#define SPI_LEDS_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

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
  };

  spi_leds_set_format(state->spi_leds, state->config->artnet_mode, data, len, params);
}

static void spi_leds_artnet_main(void *ctx)
{
  struct spi_leds_state *state = ctx;

  for (;;) {
    uint32_t notify_bits;
    bool unsync = false;

    if (!xTaskNotifyWait(0, ARTNET_OUTPUT_TASK_INDEX_BITS | ARTNET_OUTPUT_TASK_SYNC_BIT, &notify_bits, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
      continue;
    }

    LOG_DEBUG("notify index=%04x sync=%d", (notify_bits & ARTNET_OUTPUT_TASK_INDEX_BITS), !!(notify_bits & ARTNET_OUTPUT_TASK_SYNC_BIT));

    for (uint8_t index = 0; index < state->artnet.universe_count; index++) {
      if (!(notify_bits & 1 << index)) {
        continue;
      }

      if (!xQueueReceive(state->artnet.queues[index], state->artnet.dmx, portMAX_DELAY)) {
        LOG_WARN("xQueueReceive");
        continue;
      }

      spi_leds_artnet_out(state, index, state->artnet.dmx);

      if (!state->artnet.dmx->sync_mode) {
        // at least one DMX update is in non-synchronous mode, so force update
        unsync = true;
      }
    }

    if (unsync || (notify_bits & ARTNET_OUTPUT_TASK_SYNC_BIT)) {
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
