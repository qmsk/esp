#pragma once

#include <cmd.h>
#include <config.h>

extern struct config config;

int init_config();

extern const struct cmdtab config_cmdtab;
