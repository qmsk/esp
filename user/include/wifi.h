#ifndef __USER_WIFI_H__
#define __USER_WIFI_H__

#include "user_config.h"
#include <cmd.h>

int init_wifi(const struct user_config *config);

void wifi_print();

extern const struct cmdtab wifi_cmdtab;

#endif
