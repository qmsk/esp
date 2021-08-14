#include "spi_leds.h"
#include "artnet.h"

#include <artnet.h>
#include <logging.h>
#include <spi_leds.h>

#define SPI_LEDS_ARTNET_TASK_NAME "spi-leds-artnet"
#define SPI_LEDS_ARTNET_TASK_STACK 1024
#define SPI_LEDS_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static void spi_leds_artnet_dmx_rgb(struct spi_leds *spi_leds, struct artnet_dmx *artnet_dmx)
{
  unsigned count = spi_leds_count(spi_leds);

  LOG_DEBUG("len=%u", artnet_dmx->len);

  for (unsigned i = 0; i < count && artnet_dmx->len >= (i + 1) * 3; i++) {
    spi_leds_set(spi_leds, i, (struct spi_led_color) {
      .r = artnet_dmx->data[i * 3 + 0],
      .g = artnet_dmx->data[i * 3 + 1],
      .b = artnet_dmx->data[i * 3 + 2],

      .parameters = {
        .brightness = 255,
      }
    });
  }
}

static void spi_leds_artnet_dmx_bgr(struct spi_leds *spi_leds, struct artnet_dmx *artnet_dmx)
{
  unsigned count = spi_leds_count(spi_leds);

  LOG_DEBUG("len=%u", artnet_dmx->len);

  for (unsigned i = 0; i < count && artnet_dmx->len >= (i + 1) * 3; i++) {
    spi_leds_set(spi_leds, i, (struct spi_led_color) {
      .b = artnet_dmx->data[i * 3 + 0],
      .g = artnet_dmx->data[i * 3 + 1],
      .r = artnet_dmx->data[i * 3 + 2],

      .parameters = {
        .brightness = 255,
      }
    });
  }
}

static void spi_leds_artnet_dmx_grb(struct spi_leds *spi_leds, struct artnet_dmx *artnet_dmx)
{
  unsigned count = spi_leds_count(spi_leds);

  LOG_DEBUG("len=%u", artnet_dmx->len);

  for (unsigned i = 0; i < count && artnet_dmx->len >= (i + 1) * 3; i++) {
    spi_leds_set(spi_leds, i, (struct spi_led_color) {
      .g = artnet_dmx->data[i * 3 + 0],
      .r = artnet_dmx->data[i * 3 + 1],
      .b = artnet_dmx->data[i * 3 + 2],

      .parameters = {
        .brightness = 255,
      }
    });
  }
}

static void spi_leds_artnet_task(void *ctx)
{
  struct spi_leds_state *state = ctx;

  for (;;) {
    if (!xQueueReceive(state->artnet.queue, state->artnet.dmx, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
      continue;
    }

    switch(state->artnet.mode) {
      case SPI_LEDS_RGB:
        spi_leds_artnet_dmx_rgb(state->spi_leds, state->artnet.dmx);
        break;

      case SPI_LEDS_BGR:
        spi_leds_artnet_dmx_bgr(state->spi_leds, state->artnet.dmx);
        break;

      case SPI_LEDS_GRB:
        spi_leds_artnet_dmx_grb(state->spi_leds, state->artnet.dmx);
        break;
    }

    if (update_spi_leds(state)) {
      LOG_WARN("update_spi_leds");
      continue;
    }
  }
}

int init_spi_leds_artnet(struct spi_leds_state *state, unsigned index, const struct spi_leds_config *config)
{
  LOG_INFO("spi-leds%u: universe=%u mode=%x", index, config->artnet_universe, config->artnet_mode);

  if (!(state->artnet.dmx = calloc(1, sizeof(*state->artnet.dmx)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  state->artnet.mode = config->artnet_mode;

  if (!(state->artnet.queue = xQueueCreate(1, sizeof(struct artnet_dmx)))) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if (add_artnet_output(config->artnet_universe, state->artnet.queue)) {
    LOG_ERROR("add_artnet_output");
    return -1;
  }

  if (xTaskCreate(&spi_leds_artnet_task, SPI_LEDS_ARTNET_TASK_NAME, SPI_LEDS_ARTNET_TASK_STACK, state, SPI_LEDS_ARTNET_TASK_PRIORITY, &state->artnet.task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("task=%p", state->artnet.task);
  }

  return 0;
}
