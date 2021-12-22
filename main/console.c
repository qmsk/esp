#include "console.h"
#include "pin_mutex.h"

#include "artnet_cmd.h"
#include "spi_leds_cmd.h"
#include "config.h"
#include "dmx_cmd.h"
#include "log.h"
#include "spiffs.h"
#include "status_leds.h"
#include "system.h"
#include "vfs.h"
#include "wifi_cmd.h"
#include "i2s_test.h"

#include <cli.h>
#include <logging.h>
#include <stdio_uart.h>
#include <uart.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

#include <stdio.h>

#if !CONFIG_NEWLIB_VFS_STDIO
# error "CONFIG_NEWLIB_VFS_STDIO is required"
#endif

// max line size
#define CLI_BUF_SIZE 512
#define CLI_MAX_ARGS 16
#define CLI_TASK_STACK 2048 // bytes
#define CLI_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

#define UART_PORT UART_0
#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 1024

struct uart *console_uart;

static struct cli *console_cli;
static xTaskHandle console_cli_task;

static struct console_config {
  bool enabled;
  uint16_t timeout;
} console_config = {
  .enabled    = true,
};

const struct configtab console_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .description = "Start UART console at boot",
    .bool_type = { .value = &console_config.enabled },
  },
  { CONFIG_TYPE_UINT16, "timeout",
    .description = "Wait [ms] timeout to open UART console",
    .uint16_type = { .value = &console_config.timeout },
  },
  {}
};

static int console_help_cmd(int argc, char **arv, void *ctx)
{
  return cli_help(console_cli);
}

static int console_exit_cmd(int argc, char **arv, void *ctx)
{
  return cli_exit(console_cli);
}

static const struct cmd cli_commands[] = {
  { "help",   console_help_cmd,     .describe = "Show commands" },
  { "exit",   console_exit_cmd,     .describe = "Exit CLI" },

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

int init_console_uart(const struct console_config *config)
{
  enum uart_port port = UART_PORT;
  int err;

  LOG_INFO("port=%d", port);

  if ((err = uart_new(&console_uart, port, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE))) {
    LOG_ERROR("uart_new");
    return err;
  }

  return 0;
}

int init_console_cli(const struct console_config *config)
{
  struct cli_options options = {
    .buf_size = CLI_BUF_SIZE,
    .max_args = CLI_MAX_ARGS,

    // this will always use the compile-time default, as it happens before config load
    .timeout  = config->timeout ? config->timeout / portTICK_PERIOD_MS : 0,
  };
  int err;

  LOG_INFO("buf_size=%u max_args=%u timeout=%u",
    options.buf_size,
    options.max_args,
    options.timeout
  );

  // interactive CLI on stdin/stdout
  if ((err = cli_init(&console_cli, cli_commands, options))) {
    LOG_ERROR("cli_init");
    return err;
  }

  return 0;
}

int init_console()
{
  const struct console_config *config = &console_config;
  int err;

  if (!config->enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = init_console_uart(config))) {
    LOG_ERROR("init_console_uart");
    return err;
  }

  if ((err = init_console_cli(config))) {
    LOG_ERROR("init_console_cli");
    return err;
  }

  return 0;
}

void stop_console_uart()
{
  LOG_INFO("closing console, use short CONFIG button press to re-start");

  stdio_detach_uart();

  if (uart_teardown(console_uart)) {
    LOG_ERROR("uart_teardown");
  }
}

void console_cli_main(void *arg)
{
  struct cli *cli = arg;
  int err;

  if ((err = cli_main(cli)) < 0) {
    LOG_ERROR("cli_main");
  } else if (err) {
    LOG_INFO("cli timeout");
  } else {
    LOG_INFO("cli exit");
  }

  // stop
  stop_console_uart();

  console_cli_task = NULL;
  vTaskDelete(NULL);
}

int start_console_uart(const struct console_config *config)
{
  struct uart_options options = {
    .clock_div    = UART_CLK_FREQ / CONFIG_ESP_CONSOLE_UART_BAUDRATE,
    .data_bits    = UART_DATA_BITS_8,
    .parity_bits  = UART_PARITY_DISABLE,
    .stop_bits    = UART_STOP_BITS_1,

    .dev_mutex    = dev_mutex[DEV_MUTEX_UART0],
    .pin_mutex    = pin_mutex[PIN_MUTEX_U0RXD],
  };
  int err;

  LOG_INFO("clock_div=%u, data_bits=%x parity=%x stop_bits=%x",
    options.clock_div,
    options.data_bits,
    options.parity_bits,
    options.stop_bits
  );

  if ((err = uart_setup(console_uart, options))) {
    LOG_ERROR("uart_setup");
    return err;
  }

  stdio_attach_uart(console_uart);

  return 0;
}

int start_console()
{
  const struct console_config *config = &console_config;
  int err;

  if (!config->enabled) {
    return 0;
  }

  if (console_cli_task) {
    LOG_WARN("running: task=%p", console_cli_task);
    return 0;
  }

  if ((err = start_console_uart(config))) {
    LOG_ERROR("start_console_uart");
    return err;
  }

  // unbuffered input
  setvbuf(stdin, NULL, _IONBF, 0);

  // line-buffered output
  setvbuf(stdout, NULL, _IOLBF, 0);

  // set line read timeout
  cli_set_timeout(console_cli, config->timeout ? config->timeout / portTICK_PERIOD_MS : 0);

  // start task
  if (xTaskCreate(&console_cli_main, "console-cli", CLI_TASK_STACK, console_cli, CLI_TASK_PRIORITY, &console_cli_task) <= 0) {
    LOG_ERROR("xTaskCreate cli");
    return -1;
  }

  return 0;
}

bool is_console_running()
{
  return console_cli_task != NULL;
}
