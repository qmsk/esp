#include "user_cmd.h"
#include "cli.h"

int test_cmd(int argc, char **argv, void *ctx)
{
  cli_printf("argc=%d:", argc);

  for (int argi = 0; argi < argc; argi++) {
    cli_printf(" %s", argv[argi]);
  }

  cli_printf("\n");

  return 0;
}

static const struct cmd user_commands[] = {
  { "help",   cli_help_cmd, .describe = "Show this listing" },
  { "test",   test_cmd, .usage = "[...]" },
  { "config", .describe = "Configuration commands", .subcommands = &config_cmdtab },
  { "wifi",   .describe = "WiFi commands", .subcommands = &wifi_cmdtab },
  { "dmx",    .describe = "DMX", .subcommands = &dmx_cmdtab },
  { "spi",    .describe = "SPI", .subcommands = &spi_cmdtab },
  {}
};

const struct cmdtab user_cmdtab = {
  .commands       = user_commands,
  .error_handler  = cli_cmd_error,
};
