#pragma once

#include <spi_leds.h>

#define P9813_STOPBYTE 0x00
#define P9813_SPI_MODE (SPI_MODE_0)

struct __attribute__((packed)) p9813_frame {
  uint8_t control;
  uint8_t b, g, r;
};

struct __attribute__((packed)) p9813_packet {
  struct p9813_frame start; // zero
  struct p9813_frame frames[];
};

int p9813_new_packet(struct p9813_packet **packetp, size_t *sizep, unsigned count);

void p9813_set_frame(struct p9813_packet *packet, unsigned index, struct spi_led_color color);
void p9813_set_frames(struct p9813_packet *packet, unsigned count, struct spi_led_color color);

unsigned p9813_count_active(struct p9813_packet *packet, unsigned count);
