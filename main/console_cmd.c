#include "console.h"
#include "console_state.h"

#include "artnet.h"
#include "config.h"
#include "dmx.h"
#include "eth.h"
#include "leds.h"
#include "log.h"
#include "sdcard.h"
#include "spiffs.h"
#include "user_leds.h"
#include "system.h"
#include "vfs.h"
#include "wifi.h"

#include <cli.h>

// cli commands
static int console_help_cmd(int argc, char **arv, void *ctx)
{
  return cli_help(console_cli);
}

static int console_exit_cmd(int argc, char **arv, void *ctx)
{
  return cli_exit(console_cli);
}

const struct cmd console_cli_commands[] = {
  { "help",   console_help_cmd,     .describe = "Show commands" },
  { "exit",   console_exit_cmd,     .describe = "Exit CLI" },

  { "user-leds",  .describe = "User LEDs",
      .subcommands = &user_leds_cmdtab,
  },
  { "spiffs",       .describe = "SPI Flash Filesystem",
      .subcommands = &spiffs_cmdtab,
  },
  { "vfs",          .describe = "Virtual File System",
      .subcommands = &vfs_cmdtab,
  },
  { "system",       .describe = "Operating System",
      .subcommands = &system_cmdtab,
  },
  { "log",          .describe = "Logging",
      .subcommands = &log_cmdtab,
  },
  { "config",       .describe = "Configuration",
      .subcommands = &config_cmdtab,
  },
#if CONFIG_SDCARD_ENABLED
  { "sdcard",       .describe = "SD Card",
      .subcommands = &sdcard_cmdtab,
  },
#endif
  { "wifi",         .describe = "WiFi",
      .subcommands = &wifi_cmdtab,
  },
#if CONFIG_ETH_ENABLED
  { "eth",         .describe = "Ethernet",
      .subcommands = &eth_cmdtab,
  },
#endif
  { "artnet",       .describe = "Art-NET",
      .subcommands = &artnet_cmdtab,
  },
  { "dmx",          .describe = "DMX",
      .subcommands = &dmx_cmdtab,
  },
  { "leds",         .describe = "LED Control",
      .subcommands = &leds_cmdtab,
  },
  {}
};
