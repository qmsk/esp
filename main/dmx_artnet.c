#include "dmx.h"
#include "artnet.h"

#include <artnet.h>
#include <logging.h>

#define DMX_ARTNET_TASK_NAME_FMT "dmx%d"
#define DMX_ARTNET_TASK_STACK 1024
#define DMX_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static int dmx_artnet_dmx(struct dmx_output_state *state, struct artnet_dmx *dmx)
{
  LOG_DEBUG("len=%u", dmx->len);

  return output_dmx(state, dmx->data, dmx->len);
}

static void dmx_artnet_output_main(void *ctx)
{
  struct dmx_output_state *state = ctx;

  for (;;) {
    if (!xQueueReceive(state->artnet.queue, &state->artnet.dmx, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
    } else if (dmx_artnet_dmx(state, &state->artnet.dmx)) {
      LOG_WARN("dmx_artnet_dmx");
    }
  }
}

int dmx_artnet_output_init(struct dmx_artnet_output *output, int index, uint16_t universe)
{
  if (!(output->queue = xQueueCreate(1, sizeof(struct artnet_dmx)))) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  struct artnet_output_options options = {
    .port = (enum artnet_port) (index), // use dmx%d index as output port number
    .address = universe, // net/subnet set by add_artnet_output()
  };

  if (add_artnet_output(options, output->queue)) {
    LOG_ERROR("add_artnet_output");
    return -1;
  }

  return 0;
}

int init_dmx_artnet_output(struct dmx_output_state *state, int index, const struct dmx_output_config *config)
{
  char task_name[configMAX_TASK_NAME_LEN];
  int err;

  LOG_INFO("dmx%d: artnet_universe=%u", index, config->artnet_universe);

  if ((err = dmx_artnet_output_init(&state->artnet, index, config->artnet_universe))) {
    LOG_ERROR("dmx_artnet_output_init");
    return err;
  }

  snprintf(task_name, sizeof(task_name), DMX_ARTNET_TASK_NAME_FMT, index);

  if (xTaskCreate(&dmx_artnet_output_main, task_name, DMX_ARTNET_TASK_STACK, state, DMX_ARTNET_TASK_PRIORITY, &state->artnet.task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("task=%p", dmx_artnet->task);
  }

  return 0;
}
