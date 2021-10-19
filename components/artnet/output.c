#include "artnet.h"

#include <logging.h>

int artnet_add_output(struct artnet *artnet, uint16_t address, xQueueHandle queue)
{
  if (artnet->output_count >= ARTNET_OUTPUTS) {
    LOG_ERROR("too many outputs");
    return -1;
  }

  if ((address & 0xFFF0) != artnet->options.address) {
    LOG_ERROR("port address=%04x mismatch with artnet.universe=%04x", address, artnet->options.address);
    return -1;
  }

  LOG_INFO("port=%u address=%04x", artnet->output_count, address);

  artnet->output_ports[artnet->output_count] = (struct artnet_output){
    .type     = ARTNET_PORT_TYPE_DMX,
    .address  = address,
    .queue    = queue,
  };
  artnet->output_count++;

  return 0;
}

int artnet_add_outputs(struct artnet *artnet, uint16_t address, uint8_t index, xQueueHandle queue, xTaskHandle task)
{
  if (artnet->output_count >= ARTNET_OUTPUTS) {
    LOG_ERROR("too many outputs");
    return -1;
  }

  if ((address & 0xFFF0) != artnet->options.address) {
    LOG_ERROR("port index=%u address=%04x mismatch with artnet.universe=%04x", index, address, artnet->options.address);
    return -1;
  }

  LOG_INFO("port=%u index=%u address=%04x", artnet->output_count, index, address);

  artnet->output_ports[artnet->output_count] = (struct artnet_output){
    .type     = ARTNET_PORT_TYPE_DMX,
    .address  = address,
    .index    = index,
    .queue    = queue,
    .task     = task,
  };
  artnet->output_count++;

  return 0;
}

int artnet_get_outputs(struct artnet *artnet, struct artnet_output_info *outputs, size_t *size)
{
  // XXX: locking for concurrent artnet_add_output()?

  for (unsigned i = 0; i < artnet->output_count && i < *size; i++) {
    struct artnet_output *output = &artnet->output_ports[i];

    outputs[i] = (struct artnet_output_info) {
      .address = output->address,
      .index   = output->index,
      .seq     = output->seq,
    };
  }

  *size = artnet->output_count;

  return 0;
}

int artnet_find_output(struct artnet *artnet, uint16_t address, struct artnet_output **outputp)
{
  for (unsigned port = 0; port < artnet->output_count; port++) {
    struct artnet_output *output = &artnet->output_ports[port];

    if (output->address == address) {
      *outputp = output;
      return 0;
    }
  }

  return 1;
}

int artnet_output_dmx(struct artnet_output *output, struct artnet_dmx *dmx, uint8_t seq)
{
  if (seq == 0) {
    // reset
    output->seq = 0;

  } else if (seq <= output->seq && output->seq - seq < 128) {
    LOG_WARN("skip address=%04x seq=%d < %d", output->address, seq, output->seq);
    return 0;

  } else {
    // advance or wraparound
    output->seq = seq;
  }

  if (!xQueueOverwrite(output->queue, dmx)) {
    LOG_WARN("address=%04x seq=%d xQueueOverwrite", output->address, seq);
  }

  if (output->task) {
    xTaskNotify(output->task, (1 << output->index), eSetBits);
  }

  return 0;
}
