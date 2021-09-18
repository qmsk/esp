#pragma once

#include <cmd.h>
#include <config.h>

extern struct config config;

/*
 * Temporarily reset config for this boot.
 *
 * Must be called before init_config()
 */
void disable_config();

int init_config();

/*
 * Best attempt to reset persistent configuration for next boot.
 */
void reset_config();

extern const struct cmdtab config_cmdtab;
