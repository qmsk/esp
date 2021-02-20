#include "config.h"

#include "apa102.h"
#include "artnet_config.h"
#include "dmx_config.h"
#include "http_config.h"
#include "p9813_config.h"
#include "wifi_config.h"

#include <lib/config.h>
#include <lib/logging.h>

const struct configmod configmods[] = {
  { "wifi",
    .table = wifi_configtab,
  },
  { "http",
    .table = http_configtab,
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
  { "apa102",
    .table  = apa102_configtab,
  },
  {}
};

struct config config = {
  .filename = "config.ini",
  .modules  = configmods,
};

int init_config()
{
  LOG_INFO("Load config=%s", config.filename);

  if (config_load(&config)) {
    LOG_WARN("Load config failed");
  }

  return 0;
}

const struct cmdtab config_cmdtab = {
  .commands = config_commands,
  .arg      = &config,
};
