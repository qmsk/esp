#pragma once

#include <cmd.h>
#include <config.h>

#define LEDS_COUNT 4

extern const struct configtab leds_spi_configtab[];
extern const struct configtab leds_uart_configtab[];
extern const struct configtab *leds_configtabs[LEDS_COUNT];
extern const struct cmdtab leds_cmdtab;

int init_leds();
int start_leds();
