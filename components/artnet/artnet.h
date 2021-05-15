#pragma once

#define ARTNET_OUTPUTS 4

#include <artnet.h>
#include "protocol.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <lwip/sockets.h>

/* network.c */
struct artnet_sendrecv {
  struct sockaddr addr;
  socklen_t addrlen;

  union artnet_packet *packet;
  size_t len;
};

int artnet_listen(int *sockp, uint16_t port);
int artnet_send(int sock, const struct artnet_sendrecv *send);
int artnet_recv(int sock, struct artnet_sendrecv *recv);

/* output.c */
struct artnet_output {
  enum artnet_port_type type;
  uint16_t addr;
  uint8_t seq;

  xQueueHandle queue;
};

int artnet_find_output(struct artnet *artnet, uint16_t addr, struct artnet_output **outputp);
int artnet_output_dmx(struct artnet_output *output, struct artnet_dmx *dmx, uint8_t seq);

/* protocol.c */
int artnet_sendrecv(struct artnet *artnet, struct artnet_sendrecv *sendrecv);

/* artnet.c */
struct artnet {
  struct artnet_options options;

  struct artnet_output output_ports[ARTNET_OUTPUTS];
  unsigned output_count;

  int socket;
  union artnet_packet packet;

  xTaskHandle task;
};
