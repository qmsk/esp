#include "artnet.h"

#include <logging.h>

int artnet_add_input(struct artnet *artnet, struct artnet_input **inputp, struct artnet_input_options options)
{
  unsigned index = artnet->input_count;

  if (artnet->input_count >= artnet->options.inputs) {
    LOG_ERROR("exceeded number of supported inputs=%d", artnet->options.inputs);
    return -1;
  }

  if ((options.address & 0xFFF0) != artnet->options.address) {
    LOG_ERROR("port=%u index=%u address=%04x mismatch with artnet.universe=%04x", options.port, options.index, options.address, artnet->options.address);
    return -1;
  }

  LOG_DEBUG("input=%d port=%d index=%u address=%04x", artnet->input_count, options.port, options.index, options.address);

  struct artnet_input *input = &artnet->input_ports[artnet->input_count++];

  input->artnet = artnet;
  input->index = index;

  input->type = ARTNET_PORT_TYPE_DMX;
  input->options = options;

  if (!(input->queue = xQueueCreate(1, sizeof(struct artnet_dmx)))) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  *inputp = input;

  return 0;
}

void artnet_input_dmx(struct artnet_input *input, const struct artnet_dmx *dmx)
{
  // attempt normal send first, before overwriting for overflow stats
  if (xQueueSend(input->queue, dmx, 0) == errQUEUE_FULL) {
    // TODO: stats_counter_increment(&input->stats.queue_overwrite);

    xQueueOverwrite(input->queue, dmx);
  }

  if (input->artnet->input_task) {
    xTaskNotify(input->artnet->input_task, (1 << input->index) & ARTNET_INPUT_TASK_INDEX_BITS, eSetBits);
  }
}

int artnet_inputs_main(struct artnet *artnet)
{
  uint32_t notify_bits;

  artnet->input_task = xTaskGetCurrentTaskHandle();

  for (;;) {
    if (!xTaskNotifyWait(0, ARTNET_INPUT_TASK_INDEX_BITS, &notify_bits, portMAX_DELAY)) {
      LOG_ERROR("xTaskNotifyWait");
      continue;
    }

    for (unsigned index = 0; index < artnet->input_count; index++) {
      struct artnet_input *input = &artnet->input_ports[index];

      if (!(notify_bits & (1 << index))) {
        continue;
      }

      if (!xQueueReceive(input->queue, artnet->input_dmx, 0)) {
        LOG_WARN("xQueueReceive");
        continue;
      }

      if (artnet_outputs_dmx(artnet, input->options.address, artnet->input_dmx)) {
        LOG_WARN("artnet_outputs_dmx");
        continue;
      }
    }
  }
}
