#include "artnet.h"
#include "config.h"
#include "console.h"
#include "log.h"
#include "spiffs.h"
#include "status_leds.h"
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

  { "status-leds",  .describe = "Status LEDs",
      .subcommands = &status_leds_cmdtab,
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
  { "wifi",         .describe = "WiFi",
      .subcommands = &wifi_cmdtab,
  },
  { "artnet",         .describe = "Art-NET",
      .subcommands = &artnet_cmdtab,
  },
  {}
};
