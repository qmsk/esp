#include "artnet.h"

#include <logging.h>

static void init_output_stats(struct artnet_output *output)
{
  stats_counter_init(&output->stats.sync_recv);
  stats_counter_init(&output->stats.dmx_recv);
  stats_counter_init(&output->stats.dmx_sync);
  stats_counter_init(&output->stats.seq_skip);
  stats_counter_init(&output->stats.seq_drop);
  stats_counter_init(&output->stats.queue_overwrite);
}

int artnet_add_output(struct artnet *artnet, struct artnet_output_options options, xQueueHandle queue)
{
  if (artnet->output_count >= ARTNET_OUTPUTS) {
    LOG_ERROR("too many outputs");
    return -1;
  }


  if ((options.address & 0xFFF0) != artnet->options.address) {
    LOG_ERROR("port=%u index=%u address=%04x mismatch with artnet.universe=%04x", options.port, options.index, options.address, artnet->options.address);
    return -1;
  }

  LOG_DEBUG("output=%d port=%d index=%u address=%04x", artnet->output_count, options.port, options.index, options.address);

  struct artnet_output *output = &artnet->output_ports[artnet->output_count++];

  output->type = ARTNET_PORT_TYPE_DMX;
  output->options = options;
  output->queue = queue;

  init_output_stats(output);

  return 0;
}

unsigned artnet_get_output_count(struct artnet *artnet)
{
  return artnet->output_count;
}

int artnet_get_outputs(struct artnet *artnet, struct artnet_output_options *outputs, size_t *size)
{
  // XXX: locking for concurrent artnet_add_output()?

  for (unsigned i = 0; i < artnet->output_count && i < *size; i++) {
    struct artnet_output *output = &artnet->output_ports[i];

    outputs[i] = output->options;
  }

  *size = artnet->output_count;

  return 0;
}

int artnet_get_output_state(struct artnet *artnet, int index, struct artnet_output_state *state)
{
  if (index >= artnet->output_count) {
    return 1;
  }

  struct artnet_output *output = &artnet->output_ports[index];

  *state = output->state;

  return 0;
}

int artnet_get_output_stats(struct artnet *artnet, int index, struct artnet_output_stats *stats)
{
  if (index >= artnet->output_count) {
    return 1;
  }

  struct artnet_output *output = &artnet->output_ports[index];

  stats->sync_recv = stats_counter_copy(&output->stats.sync_recv);
  stats->dmx_recv = stats_counter_copy(&output->stats.dmx_recv);
  stats->dmx_sync = stats_counter_copy(&output->stats.dmx_sync);
  stats->seq_skip = stats_counter_copy(&output->stats.seq_skip);
  stats->seq_drop = stats_counter_copy(&output->stats.seq_drop);
  stats->queue_overwrite = stats_counter_copy(&output->stats.queue_overwrite);

  return 0;
}

int artnet_find_output(struct artnet *artnet, uint16_t address, struct artnet_output **outputp)
{
  for (unsigned port = 0; port < artnet->output_count; port++) {
    struct artnet_output *output = &artnet->output_ports[port];

    if (output->options.address == address) {
      *outputp = output;
      return 0;
    }
  }

  return 1;
}

static inline uint8_t artnet_seq_next(uint8_t seq)
{
  return (seq == 255) ? 1 : seq + 1;
}

void artnet_output_dmx(struct artnet_output *output, struct artnet_dmx *dmx)
{
  stats_counter_increment(&output->stats.dmx_recv);

  if (dmx->seq == 0 || output->state.seq == 0) {
    // init or reset

  } else if (dmx->seq == artnet_seq_next(output->state.seq)) {
    // in-order

  } else if (dmx->seq > output->state.seq || output->state.seq - dmx->seq >= 128) {
    // skipped
    stats_counter_increment(&output->stats.seq_skip);

  } else {
    LOG_WARN("drop address=%04x seq=%d < %d", output->options.address, dmx->seq, output->state.seq);

    stats_counter_increment(&output->stats.seq_drop);

    return;
  }

  // advance
  output->state.seq = dmx->seq;

  if (dmx->sync_mode) {
      stats_counter_increment(&output->stats.dmx_sync);
  }

  // attempt normal send first, before overwriting for overflow stats
  if (xQueueSend(output->queue, dmx, 0) == errQUEUE_FULL) {
    stats_counter_increment(&output->stats.queue_overwrite);

    xQueueOverwrite(output->queue, dmx);
  }

  if (output->options.task) {
    xTaskNotify(output->options.task, (1 << output->options.index) & ARTNET_OUTPUT_TASK_INDEX_BITS, eSetBits);
  }
}

int artnet_outputs_dmx(struct artnet *artnet, uint16_t address, struct artnet_dmx *dmx)
{
  bool found = 0;

  for (unsigned port = 0; port < artnet->output_count; port++) {
    struct artnet_output *output = &artnet->output_ports[port];

    if (output->options.address != address) {
      continue;
    }

    found = 1;

    artnet_output_dmx(output, dmx);
  }

  if (!found) {
    stats_counter_increment(&artnet->stats.dmx_discard);
  }

  return 0;
}

void artnet_output_sync(struct artnet_output *output)
{
  stats_counter_increment(&output->stats.sync_recv);

  if (output->options.task) {
    xTaskNotify(output->options.task, ARTNET_OUTPUT_TASK_SYNC_BIT, eSetBits);
  }
}

int artnet_sync_outputs(struct artnet *artnet)
{
  for (unsigned i = 0; i < artnet->output_count; i++) {
    struct artnet_output *output = &artnet->output_ports[i];

    artnet_output_sync(output);
  }

  return 0;
}

void artnet_output_test(struct artnet_output *output)
{
  if (output->options.task) {
    xTaskNotify(output->options.task, ARTNET_OUTPUT_TASK_TEST_BIT, eSetBits);
  }
}

int artnet_test_outputs(struct artnet *artnet)
{
  for (unsigned i = 0; i < artnet->output_count; i++) {
    struct artnet_output *output = &artnet->output_ports[i];

    artnet_output_test(output);
  }

  return 0;
}
