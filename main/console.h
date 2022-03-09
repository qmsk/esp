#pragma once

#include <cli.h>
#include <cmd.h>
#include <config.h>

int init_console();
int start_console();
bool is_console_running();

extern const struct cmd console_cli_commands[];
extern const struct configtab console_configtab[];
