#include "artnet.h"

#include <logging.h>

static void init_output_stats(struct artnet_output_stats *stats)
{
  stats_counter_init(&stats->sync_recv);
  stats_counter_init(&stats->dmx_recv);
  stats_counter_init(&stats->dmx_sync);
  stats_counter_init(&stats->seq_skip);
  stats_counter_init(&stats->seq_drop);
  stats_counter_init(&stats->seq_resync);
  stats_counter_init(&stats->queue_overwrite);
}

int artnet_add_output(struct artnet *artnet, struct artnet_output **outputp, struct artnet_output_options options)
{
  xQueueHandle queue;

  if (artnet->output_count >= artnet->output_size) {
    LOG_ERROR("too many outputs");
    return -1;
  }

  if ((options.address & 0xFFF0) != artnet->options.address) {
    LOG_WARN("address=%04x mismatch with artnet.universe=%04x, will not be discoverable", options.address, artnet->options.address);
  }

  if (!options.event_group) {
    LOG_DEBUG("event_bits unused");
  } else if (!options.event_bits) {
    LOG_ERROR("event_bits=%08x empty", options.event_bits);
    return -1;
  } else if ((options.event_bits & ~ARTNET_OUTPUT_EVENT_INDEX_BITS)) {
    LOG_ERROR("event_bits=%08x overflow", options.event_bits);
    return -1;
  } else {
    LOG_DEBUG("event_bits=%08x", options.event_bits);
  }

  LOG_DEBUG("output=%d address=%04x", artnet->output_count, options.address);

  if (!(queue = xQueueCreate(1, sizeof(struct artnet_dmx)))) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  struct artnet_output *output = &artnet->output_ports[artnet->output_count++];

  output->type = ARTNET_PORT_TYPE_DMX;
  output->options = options;
  output->queue = queue;

  init_output_stats(&output->stats);

  *outputp = output;

  return 0;
}

int artnet_output_read(struct artnet_output *output, struct artnet_dmx *dmx, TickType_t ticks)
{
  if (!xQueueReceive(output->queue, dmx, ticks)) {
    return 1;
  } else {
    return 0;
  }
}

unsigned artnet_get_output_count(struct artnet *artnet)
{
  return artnet->output_count;
}

int artnet_get_outputs(struct artnet *artnet, struct artnet_output_options *options, size_t *size)
{
  // XXX: locking for concurrent artnet_add_output()?

  for (unsigned i = 0; i < artnet->output_count && i < *size; i++) {
    struct artnet_output *output = &artnet->output_ports[i];

    options[i] = output->options;
  }

  *size = artnet->output_count;

  return 0;
}

int artnet_get_output_options(struct artnet *artnet, int index, struct artnet_output_options *options)
{
  if (index >= artnet->output_count) {
    return 1;
  }

  struct artnet_output *output = &artnet->output_ports[index];

  *options = output->options;

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

void artnet_reset_outputs_stats(struct artnet *artnet)
{
  for (unsigned i = 0; i < artnet->output_count; i++) {
    struct artnet_output *output = &artnet->output_ports[i];

    init_output_stats(&output->stats);
  }
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
  stats->seq_resync = stats_counter_copy(&output->stats.seq_resync);
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
  TickType_t tick = xTaskGetTickCount();

  stats_counter_increment(&output->stats.dmx_recv);

  if (dmx->seq == 0 || output->state.seq == 0) {
    // init or reset

  } else if (dmx->seq == artnet_seq_next(output->state.seq)) {
    // in-order

  } else if (dmx->seq > output->state.seq || output->state.seq - dmx->seq >= 128) {
    // skipped
    stats_counter_increment(&output->stats.seq_skip);

  } else if (output->state.tick < tick && (tick - output->state.tick) > ARTNET_SEQ_TICKS) {
    LOG_WARN("resync address=%04x seq=%d < %d on timeout", output->options.address, dmx->seq, output->state.seq);

    // timeout, resync to new seq
    stats_counter_increment(&output->stats.seq_resync);

    // updates new dmx->seq

  } else {
    LOG_WARN("drop address=%04x seq=%d < %d", output->options.address, dmx->seq, output->state.seq);

    stats_counter_increment(&output->stats.seq_drop);

    // do NOT update output->state.tick, in order to resync on timeout
    return;
  }

  // advance
  if (dmx->seq) {
    output->state.seq = dmx->seq;
  } else {
    output->state.seq++;
  }

  output->state.tick = tick;

  if (dmx->sync_mode) {
      stats_counter_increment(&output->stats.dmx_sync);
  }

  // attempt normal send first, before overwriting for overflow stats
  if (xQueueSend(output->queue, dmx, 0) == errQUEUE_FULL) {
    stats_counter_increment(&output->stats.queue_overwrite);

    xQueueOverwrite(output->queue, dmx);
  }

  if (output->options.event_group) {
    xEventGroupSetBits(output->options.event_group, output->options.event_bits);
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

int artnet_sync_outputs(struct artnet *artnet)
{
  EventGroupHandle_t event_group = NULL;

  for (unsigned i = 0; i < artnet->output_count; i++) {
    struct artnet_output *output = &artnet->output_ports[i];
    
    if (!output->options.event_group) {
      // sync not supported
      continue;
    }

    stats_counter_increment(&output->stats.sync_recv);

    if (output->options.event_group != event_group) {
      xEventGroupSetBits(output->options.event_group, (1 << ARTNET_OUTPUT_EVENT_SYNC_BIT));

      // only notify each event_group once
      event_group = output->options.event_group;
    }
  }

  return 0;
}
