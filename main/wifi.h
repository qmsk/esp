#pragma once

#include <cmd.h>

int init_wifi();
int init_wifi_events();

int start_wifi();
int stop_wifi();

extern const struct cmdtab wifi_cmdtab;
