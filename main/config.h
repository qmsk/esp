#pragma once

#include <config.h>
#include <cmd.h>

extern struct config config;

/*
 * Disable config load on boot.
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

/*
 * CLI
 */
extern const struct cmdtab config_cmdtab;
