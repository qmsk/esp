#include "console.h"

#include "artnet_cmd.h"
#include "spi_leds_cmd.h"
#include "config.h"
#include "dmx.h"
#include "log.h"
#include "spiffs.h"
#include "status_leds.h"
#include "system.h"
#include "vfs.h"
#include "wifi_cmd.h"

#include "i2s_test.h"

#include <logging.h>
#include <cli.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <stdio.h>

// max line size
#define CLI_BUF_SIZE 512
#define CLI_MAX_ARGS 16
#define CLI_TASK_STACK 2048 // bytes
#define CLI_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static struct cli *cli;
static xTaskHandle _cli_task;

static int help_cmd(int argc, char **arv, void *ctx)
{
  return cmd_help(cli);
}

static const struct cmd commands[] = {
  { "help",   help_cmd,   .describe = "Show commands" },
  { "log", .describe = "Logging",
      .subcommands = &log_cmdtab,
  },
  { "system", .describe = "System",
      .subcommands = &system_cmdtab,
  },
  { "status-leds", .describe = "Status LEDs",
      .subcommands = &status_leds_cmdtab,
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
  { "artnet", .describe = "ArtNet receiver",
      .subcommands = &artnet_cmdtab,
  },
  { "spi-leds", .describe = "SPI LEDs",
      .subcommands = &spi_leds_cmdtab,
  },
  { "dmx", .describe = "DMX output",
      .subcommands = &dmx_cmdtab,
  },
  { "i2s-test", .describe = "I2S Test",
      .subcommands = &i2s_test_cmdtab,
  },
  { },
};

void cli_task(void *arg)
{
  struct cli *cli = arg;
  int err;

  if ((err = cli_main(cli))) {
    LOG_ERROR("cli_main");
  }

  // stop
  vTaskDelete(NULL);
}

int init_console()
{
  // unbuffered input
  setvbuf(stdin, NULL, _IONBF, 0);

  // line-buffered output
  setvbuf(stdout, NULL, _IOLBF, 0);

  LOG_INFO("buf_size=%u max_args=%u", CLI_BUF_SIZE, CLI_MAX_ARGS);

  // interactive CLI on stdin/stdout
  if (cli_init(&cli, commands, CLI_BUF_SIZE, CLI_MAX_ARGS)) {
    LOG_ERROR("cli_init");
    return -1;
  }

  if (xTaskCreate(&cli_task, "cli", CLI_TASK_STACK, cli, CLI_TASK_PRIORITY, &_cli_task) <= 0) {
    LOG_ERROR("xTaskCreate cli");
    return -1;
  }

  return 0;
}
