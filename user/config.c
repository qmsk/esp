#include "config.h"

#include "artnet_config.h"
#include "dmx_config.h"
#include "p9813_config.h"
#include "wifi_config.h"

#include <lib/config.h>
#include <lib/logging.h>

const struct configmod user_configmods[] = {
  { "wifi",
    .table = wifi_configtab,
  },
  { "artnet",
    .table = artnet_configtab,
  },
  { "dmx",
    .table  = dmx_configtab,
  },
  { "p9813",
    .table  = p9813_configtab,
  },
  {}
};

struct config user_config = {
  .filename = "config.ini",
  .modules  = user_configmods,
};

int init_config()
{
  LOG_INFO("Load config=%s", user_config.filename);

  if (config_load(&user_config)) {
    LOG_WARN("Load config failed");
  }

  return 0;
}

const struct cmdtab config_cmdtab = {
  .commands = config_commands,
  .arg      = &user_config,
};
