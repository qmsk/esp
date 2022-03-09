#pragma once

#include <cmd.h>
#include <config.h>

#define DMX_OUTPUT_COUNT 2

extern const struct configtab dmx_uart_configtab[];
extern const struct configtab dmx_input_configtab[];
extern const struct configtab *dmx_output_configtabs[DMX_OUTPUT_COUNT];

extern const struct cmdtab dmx_cmdtab;

int init_dmx();
int start_dmx();

/*
 * Signal the DMX task to briefly release UART0, allowing it to be used by some other component.
 */
void release_dmx_uart0();
