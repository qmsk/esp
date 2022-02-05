#include "config.h"

#include <config_cmd.h>

const struct cmdtab config_cmdtab = {
  .arg      = &config,
  .commands = config_commands,
};
