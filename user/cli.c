#include "cli.h"
#include "config_cmd.h"
#include "wifi_cmd.h"
#include "dmx_cmd.h"
#include "spi_cmd.h"
#include "p9813_cmd.h"
#include "led_cmd.h"

#include <lib/cli.h>
#include <lib/logging.h>

int echo_cmd(int argc, char **argv, void *ctx)
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
  { "echo",   echo_cmd, .usage = "[...]" },
  { "led",    .describe = "Status LED", .subcommands = &led_cmdtab },
  { "config", .describe = "Configuration commands", .subcommands = &config_cmdtab },
  { "wifi",   .describe = "WiFi commands", .subcommands = &wifi_cmdtab },
  { "dmx",    .describe = "DMX", .subcommands = &dmx_cmdtab },
  { "spi",    .describe = "SPI", .subcommands = &spi_cmdtab },
  { "p9813",  .describe = "P9813", .subcommands = &p9813_cmdtab },
  {}
};

const struct cmdtab user_cmdtab = {
  .commands       = user_commands,
  .error_handler  = cli_cmd_error,
};

int init_cli()
{
  if (cli_init(&user_cmdtab)) {
    LOG_ERROR("cli_init");
    return -1;
  }

  return 0;
}
