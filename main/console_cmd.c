#include "config.h"
#include "console.h"

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

  { "config", .describe = "Configuration",
      .subcommands = &config_cmdtab,
  },
  {}
};
