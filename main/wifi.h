#pragma once

#include <cmd.h>
#include <config.h>

int init_wifi();
int init_wifi_events();

int start_wifi();
int stop_wifi();
int disable_wifi();

extern const struct cmdtab wifi_cmdtab;
extern const struct configtab wifi_configtab[];
