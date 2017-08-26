#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "user_config.h"
#include <cmd.h>

extern const struct cmd config_commands[];

int init_config(struct user_config *config);

#endif
