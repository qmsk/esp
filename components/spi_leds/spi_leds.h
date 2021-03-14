#pragma once

#include <spi_leds.h>
#include "apa102.h"

union spi_leds_packet {
  uint8_t *buf;
  struct apa102_packet *apa102;
};

struct spi_leds {
  // spi
  spi_host_t spi_host;

  // protocol
  enum spi_leds_protocol protocol;
  unsigned count;

  union spi_leds_packet packet;
  size_t packet_size;
};

int spi_leds_init(struct spi_leds *spi_leds, const struct spi_leds_options *options);
