#pragma once

#include <cmd.h>
#include <config.h>

extern struct config config;

int init_config();

/*
 * Best attempt to reset configuration for next boot.
 */
void reset_config();

extern const struct cmdtab config_cmdtab;
