#pragma once

#include <cmd.h>

int init_system();
int init_system_events();

/*
 * Soft system reset.
 */
void system_restart() __attribute__ ((noreturn));

extern const struct cmdtab system_cmdtab;
