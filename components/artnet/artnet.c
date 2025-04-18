#include "artnet.h"

#include <logging.h>
#include <stdlib.h>

void artnet_init_stats(struct artnet *artnet)
{
  stats_timer_init(&artnet->stats.recv);

  stats_counter_init(&artnet->stats.recv_error);
  stats_counter_init(&artnet->stats.recv_poll);
  stats_counter_init(&artnet->stats.recv_dmx);
  stats_counter_init(&artnet->stats.recv_sync);
  stats_counter_init(&artnet->stats.recv_unknown);
  stats_counter_init(&artnet->stats.recv_invalid);
  stats_counter_init(&artnet->stats.errors);
  stats_counter_init(&artnet->stats.dmx_discard);
}

int artnet_init(struct artnet *artnet, struct artnet_options options)
{
  int err;

  if (options.address & 0xF) {
    LOG_ERROR("address=%04X has universe bits set", options.address);
    return -1;
  }

  if (options.inputs > ARTNET_INPUTS_MAX) {
    LOG_ERROR("inputs=%u exceeds max=%u", options.inputs, ARTNET_INPUTS_MAX);
    return -1;
  }

  if (options.outputs > ARTNET_OUTPUTS_MAX) {
    LOG_ERROR("outputs=%u exceeds max=%u", options.outputs, ARTNET_OUTPUTS_MAX);
    return -1;
  }

  artnet->options = options;

  artnet_init_stats(artnet);

  if ((err = artnet_listen(&artnet->socket, options.port))) {
    LOG_ERROR("artnet_listen port=%u", options.port);
    return err;
  }

  if (options.inputs) {
    artnet->input_size = options.inputs;

    if (!(artnet->input_ports = calloc(artnet->input_size, sizeof(*artnet->input_ports)))) {
      LOG_ERROR("calloc(inputs)");
      return -1;
    }

    if (!(artnet->input_dmx = calloc(1, sizeof(*artnet->input_dmx)))) {
      LOG_ERROR("calloc(input_dmx)");
      return -1;
    }
  }

  if (options.outputs) {
    artnet->output_size = options.outputs;

    if (!(artnet->output_ports = calloc(artnet->output_size, sizeof(*artnet->output_ports)))) {
      LOG_ERROR("calloc(outputs)");
      return -1;
    }
  }

  return 0;
}

int artnet_new(struct artnet **artnetp, struct artnet_options options)
{
  struct artnet *artnet;
  int err;

  if (!(artnet = calloc(1, sizeof(*artnet)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = artnet_init(artnet, options))) {
    LOG_ERROR("artnet_init");
    free(artnet);
    return err;
  }

  *artnetp = artnet;

  return 0;
}

struct artnet_options artnet_get_options(struct artnet *artnet)
{
  return artnet->options;
}

bool artnet_get_inputs_enabled(struct artnet *artnet)
{
  return artnet->input_size > 0;
}

int artnet_set_metadata(struct artnet *artnet, const struct artnet_metadata *metadata)
{
  // XXX: locking for artnet_send_poll_reply()?
  artnet->options.metadata = *metadata;

  return 0;
}

int artnet_listen_main(struct artnet *artnet)
{
  int err;

  LOG_DEBUG("artnet=%p", artnet);

  for (;;) {
    struct artnet_sendrecv sendrecv = {
      .addrlen = sizeof(sendrecv.addr),
      .packet = &artnet->packet,
    };

    if ((err = artnet_recv(artnet->socket, &sendrecv))) {
      LOG_WARN("artnet_recv");
      stats_counter_increment(&artnet->stats.recv_error);
      continue;
    }

    WITH_STATS_TIMER(&artnet->stats.recv) {
      if ((err = artnet_sendrecv(artnet, &sendrecv)) < 0) {
        LOG_ERROR("artnet_sendrecv");
        stats_counter_increment(&artnet->stats.errors);
      } else if (err) {
        LOG_WARN("artnet_sendrecv");
        stats_counter_increment(&artnet->stats.recv_invalid);
      }
    }
  }
}

void artnet_reset_stats(struct artnet *artnet)
{
  artnet_init_stats(artnet);
  artnet_reset_inputs_stats(artnet);
  artnet_reset_outputs_stats(artnet);
}

void artnet_get_stats(struct artnet *artnet, struct artnet_stats *stats)
{
  stats->recv = stats_timer_copy(&artnet->stats.recv);

  stats->recv_error = stats_counter_copy(&artnet->stats.recv_error);
  stats->recv_poll = stats_counter_copy(&artnet->stats.recv_poll);
  stats->recv_dmx = stats_counter_copy(&artnet->stats.recv_dmx);
  stats->recv_sync = stats_counter_copy(&artnet->stats.recv_sync);
  stats->recv_unknown = stats_counter_copy(&artnet->stats.recv_unknown);
  stats->recv_invalid = stats_counter_copy(&artnet->stats.recv_invalid);
  stats->errors = stats_counter_copy(&artnet->stats.errors);
  stats->dmx_discard = stats_counter_copy(&artnet->stats.dmx_discard);
}
