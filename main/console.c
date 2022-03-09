#include "console.h"
#include "console_config.h"
#include "console_state.h"
#include "dev_mutex.h"
#include "pin_mutex.h"
#include "tasks.h"

#include <logging.h>
#include <cli.h>
#include <stdio_uart.h>
#include <uart.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP8266 && !CONFIG_NEWLIB_VFS_STDIO
# error "CONFIG_NEWLIB_VFS_STDIO is required"
#elif !CONFIG_IDF_TARGET_ESP8266 && !CONFIG_VFS_USE_STDIO
# error console requires VFS_USE_STDIO enabled
#endif

// kconfig ESP_CONSOLE_UART_* determines the UART used for sdk/esp_log output
// this is always hardcoded to use the same USB <-> UART0 used for bootloader flashing
// stopping the console allows using the UART0 for IO
#define UART_PORT (UART_0)
#define UART_BAUD_RATE (CONFIG_ESP_CONSOLE_UART_BAUDRATE)

// uart settings
#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 1024

// max line size
#define CLI_BUF_SIZE 512
#define CLI_MAX_ARGS 16

struct cli *console_cli;

struct uart *console_uart;
static xTaskHandle console_cli_task;

int init_console_uart()
{
  uart_port_t port = UART_PORT;
  int err;

  LOG_INFO("port=%d", port);

  if ((err = uart_new(&console_uart, port, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE))) {
    LOG_ERROR("uart_new");
    return err;
  }

  return 0;
}

int init_console_cli()
{
  const struct console_config *config = &console_config;
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
  if ((err = cli_init(&console_cli, console_cli_commands, options))) {
    LOG_ERROR("cli_init");
    return err;
  }

  return 0;
}

int init_console()
{
  int err;

  if ((err = init_console_uart())) {
    LOG_ERROR("init_console_uart");
    return err;
  }

  if ((err = init_console_cli())) {
    LOG_ERROR("init_console_cli");
    return err;
  }

  return 0;
}

int start_console_uart()
{
  struct uart_options options = {
    .baud_rate    = UART_BAUD_RATE,
    .data_bits    = UART_DATA_8_BITS,
    .parity_bits  = UART_PARITY_DISABLE,
    .stop_bits    = UART_STOP_BITS_1,

    .dev_mutex    = dev_mutex[DEV_MUTEX_UART0],
    .pin_mutex    = pin_mutex[PIN_MUTEX_U0RXD],
  };
  int err;

  LOG_INFO("baud_rate=%u, data_bits=%x parity=%x stop_bits=%x",
    options.baud_rate,
    options.data_bits,
    options.parity_bits,
    options.stop_bits
  );

  if ((err = uart_setup(console_uart, options))) {
    LOG_ERROR("uart_setup");
    return err;
  }

  LOG_INFO("started");

  return 0;
}

int start_console_stdio()
{
  // hookup stdio <-> uart
  stdio_attach_uart(console_uart);

  // unbuffered input
  setvbuf(stdin, NULL, _IONBF, 0);

  // line-buffered output
  setvbuf(stdout, NULL, _IOLBF, 0);

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
  const struct console_config *config = &console_config;
  struct cli *cli = arg;
  int err;

  // NOTE: this may block waiting for other UART users to release
  if ((err = start_console_uart())) {
    LOG_ERROR("start_console_uart");
    // TODO: alert?
    goto exit;
  }

  // NOTE: this may block waiting for other UART users to release
  if ((err = start_console_stdio())) {
    LOG_ERROR("start_console_stdio");
    // TODO: alert?
    goto exit;
  }

  // set line read timeout
  cli_set_timeout(cli, config->timeout ? config->timeout / portTICK_PERIOD_MS : 0);

  if ((err = cli_main(cli)) < 0) {
    LOG_ERROR("cli_main");
  } else if (err) {
    LOG_INFO("cli timeout");
  } else {
    LOG_INFO("cli exit");
  }

  // stop
  stop_console_uart();

exit:
  console_cli_task = NULL;
  vTaskDelete(NULL);
}

int start_console()
{
  const struct console_config *config = &console_config;
  struct task_options task_options = {
    .main       = console_cli_main,
    .name       = CONSOLE_CLI_TASK_NAME,
    .stack_size = CONSOLE_CLI_TASK_STACK,
    .arg        = console_cli,
    .priority   = CONSOLE_CLI_TASK_PRIORITY,
    .handle     = &console_cli_task,
    .affinity   = CONSOLE_CLI_TASK_AFFINITY,
  };

  if (!config->enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if (console_cli_task) {
    LOG_WARN("running: task=%p", console_cli_task);
    return 1;
  } else if (start_task(task_options)) {
    LOG_ERROR("start_task");
    return -1;
  } else {
    LOG_INFO("task=%p", console_cli_task);
  }

  return 0;
}
