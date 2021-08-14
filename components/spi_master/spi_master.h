#pragma once

#include <spi_master.h>

struct spi_master {
  enum spi_mode mode;
  enum spi_clock clock;
};

int spi_master_init(struct spi_master *spi_master, const struct spi_options options);

int spi_master_mode(struct spi_master *spi_master, enum spi_mode mode);
int spi_master_clock(struct spi_master *spi_master, enum spi_clock clock);
int spi_master_pins(struct spi_master *spi_master, enum spi_pins pins);
