#pragma once

#include <cli.h>
#include <cmd.h>

extern struct cli *console_cli;

int init_console();
int start_console();

extern const struct cmd console_cli_commands[];
