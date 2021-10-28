#pragma once

#include <spi_leds.h>

struct __attribute__((packed)) apa102_frame {
  uint8_t global;
  uint8_t b, g, r;
};

struct __attribute__((packed)) apa102_packet {
  struct apa102_frame start; // zero
  struct apa102_frame frames[];
};

struct spi_leds_protocol_apa102 {
  struct apa102_packet *packet;
  size_t packet_size;
};

int spi_leds_init_apa102(struct spi_leds_protocol_apa102 *protocol, const struct spi_leds_options *options);
int spi_leds_tx_apa102(struct spi_leds_protocol_apa102 *protocol, const struct spi_leds_options *options);

void apa102_set_frame(struct spi_leds_protocol_apa102 *protocol, unsigned index, struct spi_led_color color);
void apa102_set_frames(struct spi_leds_protocol_apa102 *protocol, unsigned count, struct spi_led_color color);

unsigned apa102_count_active(struct spi_leds_protocol_apa102 *protocol, unsigned count);
