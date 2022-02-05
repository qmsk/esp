#pragma once

#include "cmd.h"

int config_cmd_show(int argc, char **argv, void *ctx);
int config_cmd_get(int argc, char **argv, void *ctx);
int config_cmd_set(int argc, char **argv, void *ctx);
int config_cmd_clear(int argc, char **argv, void *ctx);
int config_cmd_reset(int argc, char **argv, void *ctx);

extern const struct cmd config_commands[];
