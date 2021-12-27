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

/*
 * initialize config SPIFFS partition, and load config.
 *
 * Returns <0 on error, 0 on success, >1 if no config loaded.
 */
int init_config();

/*
 * Best attempt to reset persistent configuration for next boot.
 */
void reset_config();

extern const struct cmdtab config_cmdtab;
