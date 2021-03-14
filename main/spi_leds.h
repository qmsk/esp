#pragma once

#include <cmd.h>
#include <config.h>

extern const struct configtab spi_leds_configtab[];

int init_spi_leds();

extern const struct cmdtab spi_leds_cmdtab;
