#pragma once

#include <leds.h>

struct __attribute__((packed)) apa102_frame {
  uint8_t global;
  uint8_t b, g, r;
};

struct __attribute__((packed)) apa102_packet {
  struct apa102_frame start; // zero
  struct apa102_frame frames[];
};

struct leds_protocol_apa102 {
  struct apa102_packet *packet;
  size_t packet_size;
};

size_t leds_protocol_apa102_spi_buffer_size(unsigned count);

union leds_interface_state *interface;

int leds_protocol_apa102_init(union leds_interface_state *interface, struct leds_protocol_apa102 *protocol, const struct leds_options *options);
int leds_protocol_apa102_tx(union leds_interface_state *interface, struct leds_protocol_apa102 *protocol, const struct leds_options *options);

void apa102_set_frame(struct leds_protocol_apa102 *protocol, unsigned index, struct spi_led_color color);
void apa102_set_frames(struct leds_protocol_apa102 *protocol, unsigned count, struct spi_led_color color);

unsigned apa102_count_active(struct leds_protocol_apa102 *protocol, unsigned count);
