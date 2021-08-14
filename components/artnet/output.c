#include "artnet.h"

#include <logging.h>

int artnet_add_output(struct artnet *artnet, uint16_t addr, xQueueHandle queue)
{
  if (artnet->output_count >= ARTNET_OUTPUTS) {
    LOG_ERROR("too many outputs");
    return -1;
  }

  if ((addr & 0xFFF0) != artnet->options.universe) {
    LOG_WARN("port address=%04x mismatch with artnet.universe=%04x", addr, artnet->options.universe);
  }

  LOG_INFO("port=%u addr=%u", artnet->output_count, addr);

  artnet->output_ports[artnet->output_count] = (struct artnet_output){
    .addr  = addr,
    .type  = ARTNET_PORT_TYPE_DMX,
    .queue = queue,
  };
  artnet->output_count++;

  return 0;
}

int artnet_find_output(struct artnet *artnet, uint16_t addr, struct artnet_output **outputp)
{
  for (unsigned port = 0; port < artnet->output_count; port++) {
    struct artnet_output *output = &artnet->output_ports[port];

    if (output->addr == addr) {
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
    LOG_WARN("skip addr=%d seq=%d < %d", output->addr, seq, output->seq);
    return 0;

  } else {
    // advance or wraparound
    output->seq = seq;
  }

  if (!xQueueOverwrite(output->queue, dmx)) {
    LOG_WARN("addr=%u seq=%d xQueueOverwrite", output->addr, seq);
  }

  return 0;
}
