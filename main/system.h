#pragma once

#include <cmd.h>

extern const char system_boardconfig[]; // from CMake BOARDCONFIG

extern const struct cmdtab system_cmdtab;

int init_system();

/*
 * Soft system reset.
 */
void system_restart() __attribute__ ((noreturn));
