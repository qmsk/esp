#include "artnet.h"

#include <logging.h>

static void init_input_stats(struct artnet_input_stats *stats)
{
  stats_counter_init(&stats->dmx_recv);
  stats_counter_init(&stats->queue_overwrite);
}

int artnet_add_input(struct artnet *artnet, struct artnet_input **inputp, struct artnet_input_options options)
{
  unsigned index = artnet->input_count;

  if (artnet->input_count >= artnet->input_size) {
    LOG_ERROR("exceeded number of supported inputs=%d", artnet->input_size);
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

  init_input_stats(&input->stats);

  if (!(input->queue = xQueueCreate(1, sizeof(struct artnet_dmx)))) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  *inputp = input;

  return 0;
}

void artnet_input_dmx(struct artnet_input *input, const struct artnet_dmx *dmx)
{
  stats_counter_increment(&input->stats.dmx_recv);

  input->state.tick = xTaskGetTickCount();
  input->state.len = dmx->len;

  // attempt normal send first, before overwriting for overflow stats
  if (xQueueSend(input->queue, dmx, 0) == errQUEUE_FULL) {
    stats_counter_increment(&input->stats.queue_overwrite);

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

unsigned artnet_get_input_count(struct artnet *artnet)
{
  return artnet->input_count;
}

int artnet_get_inputs(struct artnet *artnet, struct artnet_input_options *options, size_t *size)
{
  // XXX: locking for concurrent artnet_add_input()?

  for (unsigned i = 0; i < artnet->input_count && i < *size; i++) {
    struct artnet_input *input = &artnet->input_ports[i];

    options[i] = input->options;
  }

  *size = artnet->input_count;

  return 0;
}

int artnet_get_input_options(struct artnet *artnet, int index, struct artnet_input_options *options)
{
  if (index >= artnet->input_count) {
    return 1;
  }

  struct artnet_input *input = &artnet->input_ports[index];

  *options = input->options;

  return 0;
}

int artnet_get_input_state(struct artnet *artnet, int index, struct artnet_input_state *state)
{
  if (index >= artnet->input_count) {
    return 1;
  }

  struct artnet_input *input = &artnet->input_ports[index];

  *state = input->state;

  return 0;
}

int artnet_get_input_stats(struct artnet *artnet, int index, struct artnet_input_stats *stats)
{
  if (index >= artnet->input_count) {
    return 1;
  }

  struct artnet_input *input = &artnet->input_ports[index];

  stats->dmx_recv = stats_counter_copy(&input->stats.dmx_recv);
  stats->queue_overwrite = stats_counter_copy(&input->stats.queue_overwrite);

  return 0;
}
