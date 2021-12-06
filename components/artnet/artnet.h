#pragma once

#define ARTNET_OUTPUTS 16

#define ARTNET_INPUTS_MAX 16
#define ARTNET_INPUT_TASK_INDEX_BITS 0xffff

// 4s timeout for sync mode
#define ARTNET_SYNC_TICKS (4000 / portTICK_PERIOD_MS)

#include <artnet.h>
#include <artnet_stats.h>
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

/* input.c */
struct artnet_input {
  struct artnet *artnet;
  unsigned index;

  enum artnet_port_type type;
  struct artnet_input_options options;

  xQueueHandle queue;

  struct artnet_input_stats stats;
};

/* output.c */
struct artnet_output {
  enum artnet_port_type type;
  struct artnet_output_options options;
  struct artnet_output_state state;

  xQueueHandle queue;

  struct artnet_output_stats stats;
};

int artnet_find_output(struct artnet *artnet, uint16_t address, struct artnet_output **outputp);
int artnet_outputs_dmx(struct artnet *artnet, uint16_t address, struct artnet_dmx *dmx);
int artnet_outputs_sync(struct artnet *artnet);

/* protocol.c */
int artnet_sendrecv(struct artnet *artnet, struct artnet_sendrecv *sendrecv);

/* artnet.c */
struct artnet {
  struct artnet_options options;

  struct artnet_input *input_ports;
  unsigned input_count;

  struct artnet_output output_ports[ARTNET_OUTPUTS];
  unsigned output_count;

  /* network */
  int socket;
  union artnet_packet packet;
  struct artnet_dmx dmx;

  /* inputs */
  xTaskHandle input_task;
  struct artnet_dmx *input_dmx;

  // last sync received at
  TickType_t sync_tick;

  struct artnet_stats stats;
};
