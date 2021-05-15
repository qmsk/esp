#include "dmx.h"
#include "artnet.h"

#include <artnet.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define DMX_ARTNET_TASK_NAME "dmx-artnet"
#define DMX_ARTNET_TASK_STACK 1024
#define DMX_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

struct dmx_artnet {
  xTaskHandle task;
  xQueueHandle queue;

  struct artnet_dmx artnet_dmx;
} *dmx_artnet;

static int dmx_artnet_dmx(struct dmx_artnet *dmx_artnet, struct artnet_dmx *artnet_dmx)
{
  LOG_DEBUG("len=%u", artnet_dmx->len);

  return output_dmx(artnet_dmx->data, artnet_dmx->len);
}

static void dmx_artnet_task(void *ctx)
{
  struct dmx_artnet *dmx_artnet = ctx;

  for (;;) {
    if (!xQueueReceive(dmx_artnet->queue, &dmx_artnet->artnet_dmx, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
    } else if (dmx_artnet_dmx(dmx_artnet, &dmx_artnet->artnet_dmx)) {
      LOG_WARN("dmx_artnet_dmx");
    }
  }
}

int dmx_artnet_init(struct dmx_artnet *dmx_artnet, uint16_t universe)
{
  if (!(dmx_artnet->queue = xQueueCreate(1, sizeof(struct artnet_dmx)))) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if (xTaskCreate(&dmx_artnet_task, DMX_ARTNET_TASK_NAME, DMX_ARTNET_TASK_STACK, dmx_artnet, DMX_ARTNET_TASK_PRIORITY, &dmx_artnet->task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("task=%p", dmx_artnet->task);
  }

  if (add_artnet_output(universe, dmx_artnet->queue)) {
    LOG_ERROR("add_artnet_output");
    return -1;
  }

  return 0;
}

int init_dmx_artnet(uint16_t universe)
{
  LOG_INFO("universe=%u", universe);

  if (!(dmx_artnet = calloc(1, sizeof(*dmx_artnet)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  return dmx_artnet_init(dmx_artnet, universe);
}
