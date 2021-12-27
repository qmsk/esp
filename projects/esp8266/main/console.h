#pragma once

#include <config.h>

extern const struct configtab console_configtab[];

int init_console();

int start_console();

bool is_console_running();
