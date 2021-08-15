#include "artnet.h"

#include <logging.h>
#include <stdlib.h>

uint16_t artnet_address(uint16_t net, uint16_t subnet, uint16_t uni)
{
  return ((net << 8) & 0x7F00) | ((subnet << 4) & 0x00F0) | (uni & 0x000F);
}

int artnet_init(struct artnet *artnet, struct artnet_options options)
{
  int err;

  LOG_INFO("port=%u address=%04x", options.port, options.address);

  if (options.address & 0xF) {
    LOG_ERROR("address=%04X has universe bits set", options.address);
    return -1;
  }

  artnet->options = options;

  if ((err = artnet_listen(&artnet->socket, options.port))) {
    LOG_ERROR("artnet_listen port=%u", options.port);
    return err;
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

int artnet_main(struct artnet *artnet)
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
      return err;
    } else if ((err = artnet_sendrecv(artnet, &sendrecv))) {
      LOG_WARN("artnet_sendrecv");
      continue;
    }
  }
}
