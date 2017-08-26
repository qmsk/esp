#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "user_config.h"
#include "cli.h"

extern const struct cli_command config_commands[];

int init_config(struct user_config *config);

#endif
