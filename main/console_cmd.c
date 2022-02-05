#include "config.h"
#include "console.h"
#include "spiffs.h"
#include "system.h"
#include "vfs.h"

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

  { "spiffs",   .describe = "SPI Flash Filesystem",
      .subcommands = &spiffs_cmdtab,
  },
  { "vfs",      .describe = "Virtual File System",
      .subcommands = &vfs_cmdtab,
  },
  { "system",   .describe = "Operating System",
      .subcommands = &system_cmdtab,
  },
  { "config", .describe = "Configuration",
      .subcommands = &config_cmdtab,
  },
  {}
};
