#ifndef __USER_CLI_H__
#define __USER_CLI_H__

#include "user_config.h"
#include <cmd.h>

int init_cli(struct user_config *config, const struct cmdtab *commands);

#endif
