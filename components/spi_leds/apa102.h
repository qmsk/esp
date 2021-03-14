#pragma once

#include <spi_leds.h>

#define APA102_STOPBYTE 0xff

struct __attribute__((packed)) apa102_frame {
  uint8_t global;
  uint8_t b, g, r;
};

struct __attribute__((packed)) apa102_packet {
  struct apa102_frame start; // zero
  struct apa102_frame frames[];
};

int apa102_new_packet(struct apa102_packet **packetp, size_t *sizep, unsigned count);

void apa102_set_frame(struct apa102_packet *packet, unsigned index, struct spi_led_color color);
void apa102_set_frames(struct apa102_packet *packet, unsigned count, struct spi_led_color color);

unsigned apa102_count_active(struct apa102_packet *packet, unsigned count);
