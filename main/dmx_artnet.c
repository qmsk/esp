#include "dmx.h"
#include "artnet.h"

#include <artnet.h>
#include <logging.h>

#define DMX_ARTNET_TASK_NAME "dmx-artnet"
#define DMX_ARTNET_TASK_STACK 1024
#define DMX_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static int dmx_artnet_dmx(struct dmx_state *state, struct artnet_dmx *artnet_dmx)
{
  LOG_DEBUG("len=%u", artnet_dmx->len);

  return output_dmx(state, artnet_dmx->data, artnet_dmx->len);
}

static void dmx_artnet_task(void *ctx)
{
  struct dmx_state *state = ctx;

  for (;;) {
    if (!xQueueReceive(state->artnet.queue, &state->artnet.artnet_dmx, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
    } else if (dmx_artnet_dmx(state, &state->artnet.artnet_dmx)) {
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

int init_dmx_artnet(struct dmx_state *state, int index, const struct dmx_config *config)
{
  LOG_INFO("%d: artnet_universe=%u", index, config->artnet_universe);

  return dmx_artnet_init(&state->artnet, config->artnet_universe);
}
