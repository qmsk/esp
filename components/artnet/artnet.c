#include "artnet.h"

#include <logging.h>
#include <stdlib.h>

#define ARTNET_TASK_NAME "artnet"
#define ARTNET_TASK_STACK 1024
#define ARTNET_TASK_PRIORITY tskIDLE_PRIORITY + 2

int artnet_init(struct artnet *artnet, struct artnet_options options)
{
  int err;

  artnet->options = options;

  if ((err = artnet_listen(&artnet->socket, options.port))) {
    LOG_ERROR("artnet_listen port=%u", options.port);
    return err;
  }

  LOG_INFO("port=%u universe=%u", options.port, options.universe);

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

void artnet_task(void *arg)
{
  struct artnet *artnet = arg;
  int err;

  LOG_DEBUG("artnet=%p", artnet);

  for (;;) {
    struct artnet_sendrecv sendrecv = {
      .addrlen = sizeof(sendrecv.addr),
      .packet = &artnet->packet,
    };

    if ((err = artnet_recv(artnet->socket, &sendrecv))) {
      LOG_WARN("artnet_recv");
    } else if ((err = artnet_sendrecv(artnet, &sendrecv))) {
      LOG_WARN("artnet_sendrecv");
    }
  }
}

int artnet_start(struct artnet *artnet)
{
  int err;

  if ((err = xTaskCreate(&artnet_task, ARTNET_TASK_NAME, ARTNET_TASK_STACK, artnet, ARTNET_TASK_PRIORITY, &artnet->task)) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("artnet task=%p", artnet->task);
  }

  return 0;
}
