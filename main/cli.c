#include "cli.h"

#include "spi_leds.h"
#include "config.h"
#include "dmx.h"
#include "spiffs.h"
#include "system.h"
#include "user_led.h"
#include "vfs.h"
#include "wifi.h"

#include <logging.h>
#include <cli.h>
#include <esp_log.h>

#include <stdio.h>

// max line size
#define CLI_BUF_SIZE 256

static struct cli *cli;

static int help_cmd(int argc, char **arv, void *ctx)
{
  return cmd_help(cli);
}

static const struct cmd commands[] = {
  { "help",   help_cmd,   .describe = "Show commands" },
  { "system", .describe = "System",
      .subcommands = &system_cmdtab,
  },
  { "user-led", .describe = "User LED",
      .subcommands = &user_led_cmdtab,
  },
  { "spiffs", .describe = "SPIFFS",
      .subcommands = &spiffs_cmdtab,
  },
  { "vfs", .describe = "Virtual FileSystem",
      .subcommands = &vfs_cmdtab,
  },
  { "config", .describe = "Configuration",
      .subcommands = &config_cmdtab,
  },
  { "wifi", .describe = "WiFi",
      .subcommands = &wifi_cmdtab,
  },
  { "spi-leds", .describe = "SPI LEDs",
      .subcommands = &spi_leds_cmdtab,
  },
  { "dmx", .describe = "DMX output",
      .subcommands = &dmx_cmdtab,
  },
  { },
};

int init_cli()
{
  // unbuffered input
  setvbuf(stdin, NULL, _IONBF, 0);

  // line-buffered output
  setvbuf(stdout, NULL, _IOLBF, 0);

  // interactive CLI on stdin/stdout
  if (cli_init(&cli, commands, CLI_BUF_SIZE)) {
    LOG_ERROR("cli_init");
    return -1;
  }

  return 0;
}
