#pragma once

#include <leds.h>

struct __attribute__((packed)) p9813_frame {
  uint8_t control;
  uint8_t b, g, r;
};

struct __attribute__((packed)) p9813_packet {
  struct p9813_frame start; // zero
  struct p9813_frame frames[];
};

struct leds_protocol_p9813 {
  struct p9813_packet *packet;
  size_t packet_size;
};

size_t leds_protocol_p9813_spi_buffer_size(unsigned count);

union leds_interface_state *interface;

int leds_protocol_p9813_init(union leds_interface_state *interface, struct leds_protocol_p9813 *protocol, const struct leds_options *options);
int leds_protocol_p9813_tx(union leds_interface_state *interface, struct leds_protocol_p9813 *protocol, const struct leds_options *options);

void p9813_set_frame(struct leds_protocol_p9813 *protocol, unsigned index, struct spi_led_color color);
void p9813_set_frames(struct leds_protocol_p9813 *protocol, unsigned count, struct spi_led_color color);

unsigned p9813_count_active(struct leds_protocol_p9813 *protocol, unsigned count);
