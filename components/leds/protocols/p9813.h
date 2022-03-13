#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../interface.h"

struct p9813_packet;

struct leds_protocol_p9813 {
  struct p9813_packet *packet;
  size_t packet_size;
  unsigned count;
};

size_t leds_protocol_p9813_spi_buffer_size(unsigned count);

int leds_protocol_p9813_init(struct leds_protocol_p9813 *protocol, union leds_interface_state *interface, const struct leds_options *options);
int leds_protocol_p9813_tx(struct leds_protocol_p9813 *protocol, union leds_interface_state *interface, const struct leds_options *options);

void leds_protocol_p9813_set(struct leds_protocol_p9813 *protocol, unsigned index, struct leds_color color);
void leds_protocol_p9813_set_all(struct leds_protocol_p9813 *protocol, struct leds_color color);

unsigned leds_protocol_p9813_count_active(struct leds_protocol_p9813 *protocol);
unsigned leds_protocol_p9813_count_total(struct leds_protocol_p9813 *protocol);
