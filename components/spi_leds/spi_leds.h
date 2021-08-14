#pragma once

#include <spi_leds.h>
#include "apa102.h"
#include "p9813.h"

#include <stdbool.h>

union spi_leds_packet {
  uint8_t *buf;
  struct apa102_packet *apa102;
  struct p9813_packet *p9813;
};

struct spi_leds {
  struct spi_leds_options options;

  // spi
  struct spi_master *spi_master;

  // protocol
  enum spi_mode spi_mode;
  union spi_leds_packet packet;
  size_t packet_size;

  // ifa false, all leds are inactive
  bool active;
};

int spi_leds_init(struct spi_leds *spi_leds, struct spi_master *spi_master, const struct spi_leds_options options);
