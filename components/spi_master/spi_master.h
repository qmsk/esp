#pragma once

#include <spi_master.h>

struct spi_master {

};

int spi_master_init(struct spi_master *spi_master, const struct spi_options options);
