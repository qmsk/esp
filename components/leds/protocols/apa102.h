#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../limit.h"

struct apa102_packet *packet;

struct leds_protocol_apa102 {
  struct apa102_packet *packet;
  size_t packet_size;
  unsigned count;
};

size_t leds_protocol_apa102_spi_buffer_size(unsigned count);

int leds_protocol_apa102_init(union leds_interface_state *interface, struct leds_protocol_apa102 *protocol, const struct leds_options *options);
int leds_protocol_apa102_tx(union leds_interface_state *interface, struct leds_protocol_apa102 *protocol, const struct leds_options *options);

void leds_protocol_apa102_set(struct leds_protocol_apa102 *protocol, unsigned index, struct leds_color color);
void leds_protocol_apa102_set_all(struct leds_protocol_apa102 *protocol, struct leds_color color);

unsigned leds_protocol_apa102_count_active(struct leds_protocol_apa102 *protocol);
unsigned leds_protocol_apa102_count_total(struct leds_protocol_apa102 *protocol);
