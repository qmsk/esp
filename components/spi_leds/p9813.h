#pragma once

#include <spi_leds.h>

struct __attribute__((packed)) p9813_frame {
  uint8_t control;
  uint8_t b, g, r;
};

struct __attribute__((packed)) p9813_packet {
  struct p9813_frame start; // zero
  struct p9813_frame frames[];
};

struct spi_leds_protocol_p9813 {
  struct p9813_packet *packet;
  size_t packet_size;
};

int spi_leds_init_p9813(struct spi_leds_protocol_p9813 *protocol, const struct spi_leds_options *options);
int spi_leds_tx_p9813(struct spi_leds_protocol_p9813 *protocol, const struct spi_leds_options *options);

void p9813_set_frame(struct spi_leds_protocol_p9813 *protocol, unsigned index, struct spi_led_color color);
void p9813_set_frames(struct spi_leds_protocol_p9813 *protocol, unsigned count, struct spi_led_color color);

unsigned p9813_count_active(struct spi_leds_protocol_p9813 *protocol, unsigned count);
