#pragma once

#include <cmd.h>

extern const struct cmdtab system_cmdtab;

/*
 * Get WiFi AP/STA hostname.
 *
 * @return <0 on error, 1 if not available
 */
int get_system_hostname(const char **hostnamep);
