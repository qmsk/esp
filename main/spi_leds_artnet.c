#include "spi_leds.h"
#include "artnet.h"

#include <artnet.h>
#include <logging.h>
#include <spi_leds.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define SPI_LEDS_ARTNET_TASK_NAME "spi-leds-artnet"
#define SPI_LEDS_ARTNET_TASK_STACK 1024
#define SPI_LEDS_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

extern struct spi_leds *spi_leds;

struct spi_leds_artnet {
  xTaskHandle task;
  xQueueHandle queue;

  struct spi_leds *spi_leds;
  struct artnet_dmx artnet_dmx;
} *spi_leds_artnet;

static int spi_leds_artnet_dmx(struct spi_leds *spi_leds, struct artnet_dmx *artnet_dmx)
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

  return update_spi_leds();
}

static void spi_leds_artnet_task(void *ctx)
{
  struct spi_leds_artnet *spi_leds_artnet = ctx;

  for (;;) {
    if (!xQueueReceive(spi_leds_artnet->queue, &spi_leds_artnet->artnet_dmx, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
    } else if (spi_leds_artnet_dmx(spi_leds_artnet->spi_leds, &spi_leds_artnet->artnet_dmx)) {
      LOG_WARN("spi_leds_artnet_dmx");
    }
  }
}

int init_spi_leds_artnet(uint16_t universe)
{
  LOG_INFO("universe=%u", universe);

  if (!(spi_leds_artnet = calloc(1, sizeof(*spi_leds_artnet)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  spi_leds_artnet->spi_leds = spi_leds;

  if (!(spi_leds_artnet->queue = xQueueCreate(1, sizeof(struct artnet_dmx)))) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if (add_artnet_output(universe, spi_leds_artnet->queue)) {
    LOG_ERROR("add_artnet_output");
    return -1;
  }

  if (xTaskCreate(&spi_leds_artnet_task, SPI_LEDS_ARTNET_TASK_NAME, SPI_LEDS_ARTNET_TASK_STACK, spi_leds_artnet, SPI_LEDS_ARTNET_TASK_PRIORITY, &spi_leds_artnet->task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("task=%p", spi_leds_artnet->task);
  }

  return 0;
}