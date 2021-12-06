#include "artnet.h"

#include <logging.h>
#include <stdlib.h>

uint16_t artnet_address(uint16_t net, uint16_t subnet, uint16_t uni)
{
  return ((net << 8) & 0x7F00) | ((subnet << 4) & 0x00F0) | (uni & 0x000F);
}

uint16_t artnet_address_net(uint16_t address)
{
  return (address & 0x7F00) >> 8;
}

uint16_t artnet_address_subnet(uint16_t address)
{
  return (address & 0x00F0) >> 4;
}

uint16_t artnet_address_universe(uint16_t address)
{
  return (address & 0x000F);
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

  artnet->options = options;

  stats_counter_init(&artnet->stats.recv_error);
  stats_counter_init(&artnet->stats.recv_poll);
  stats_counter_init(&artnet->stats.recv_dmx);
  stats_counter_init(&artnet->stats.recv_sync);
  stats_counter_init(&artnet->stats.recv_unknown);
  stats_counter_init(&artnet->stats.recv_invalid);
  stats_counter_init(&artnet->stats.errors);
  stats_counter_init(&artnet->stats.dmx_discard);

  if ((err = artnet_listen(&artnet->socket, options.port))) {
    LOG_ERROR("artnet_listen port=%u", options.port);
    return err;
  }

  if (options.inputs) {
    if (!(artnet->input_ports = calloc(options.inputs, sizeof(*artnet->input_ports)))) {
      LOG_ERROR("calloc(inputs)");
      return -1;
    }

    if (!(artnet->input_dmx = calloc(1, sizeof(*artnet->input_dmx)))) {
      LOG_ERROR("calloc(input_dmx)");
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

int artnet_set_options(struct artnet *artnet, struct artnet_options options)
{
  // XXX: locking for artnet_send_poll_reply()?
  artnet->options = options;

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
    } else if ((err = artnet_sendrecv(artnet, &sendrecv)) < 0) {
      LOG_ERROR("artnet_sendrecv");
      stats_counter_increment(&artnet->stats.errors);
      continue;
    } else if (err) {
      LOG_WARN("artnet_sendrecv");
      stats_counter_increment(&artnet->stats.recv_invalid);
      continue;
    }
  }
}

void artnet_get_stats(struct artnet *artnet, struct artnet_stats *stats)
{
  stats->recv_error = stats_counter_copy(&artnet->stats.recv_error);
  stats->recv_poll = stats_counter_copy(&artnet->stats.recv_poll);
  stats->recv_dmx = stats_counter_copy(&artnet->stats.recv_dmx);
  stats->recv_sync = stats_counter_copy(&artnet->stats.recv_dmx);
  stats->recv_unknown = stats_counter_copy(&artnet->stats.recv_unknown);
  stats->recv_invalid = stats_counter_copy(&artnet->stats.recv_invalid);
  stats->errors = stats_counter_copy(&artnet->stats.errors);
  stats->dmx_discard = stats_counter_copy(&artnet->stats.dmx_discard);
}
