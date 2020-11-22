#ifndef __USER_CONFIG_CMD_H__
#define __USER_CONFIG_CMD_H__

#include "user_config.h"

#include <lib/cmd.h>
#include <lib/config.h>

extern const struct cmdtab config_cmdtab;

const struct cmdtab config_cmdtab = {
  .commands = config_commands,
  .arg      = &user_configmeta,
};

#endif
