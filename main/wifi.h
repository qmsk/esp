#pragma once

#include <cmd.h>
#include <config.h>

extern const struct configtab wifi_configtab[];

int init_wifi();

extern const struct cmdtab wifi_cmdtab;
