#ifndef __USER_WIFI_H__
#define __USER_WIFI_H__

#include "user_config.h"
#include "user_info.h"
#include <cmd.h>

int init_wifi(const struct user_config *config, struct user_info *info);

#endif
