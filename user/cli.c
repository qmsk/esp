#define DEBUG

#include "cli.h"
#include "uart.h"
#include "logging.h"

#include <ctype.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define CLI_RX_BUF 1024
#define CLI_TASK_STACK 512
#define UART_RX_QUEUE_SIZE 8
#define CLI_ARGS_MAX 8

struct cli {
  xTaskHandle task;
  xQueueHandle uart_rx_queue;

  char rx_buf[CLI_RX_BUF], *rx_ptr;

  const struct cli_command *commands;
} cli;

static int cli_command_split(struct cli *cli, char *line, char **argv, int arg_max)
{
  char *ptr = line;
  const char *arg = NULL;
  int argc = 0;

  for (; *ptr; ptr++) {
    if (isspace((unsigned char) *ptr)) {
      *ptr = '\0';
      LOG_DEBUG("end arg %d: %s", argc, arg);
      arg = NULL;
    } else if (!arg) {
      arg = argv[argc] = ptr;
      LOG_DEBUG("start arg %d: %s", argc, arg);
      argc++;
    }

    if (argc >= arg_max) {
      LOG_WARN("argc=%d overflow", argc);
      return -1;
    }
  }

  return argc;
}

static int cli_command_lookup(struct cli *cli, const char *argv0, const struct cli_command **commandp)
{
  for (const struct cli_command *cmd = cli->commands; cmd->command; cmd++) {
    if (strcmp(argv0, cmd->command) == 0) {
      *commandp = cmd;
      return 0;
    }
  }

  return -1;
}

static int cli_command(struct cli *cli, char *line)
{
  int argc;
  char *argv[CLI_ARGS_MAX];
  const struct cli_command *command;

  if ((argc = cli_command_split(cli, line, argv, CLI_ARGS_MAX)) < 0) {
    return -1;
  } else if (argc == 0) {
    LOG_DEBUG("skip empty line");
    return 0;
  }

  LOG_DEBUG("argc=%d argv[0]=%s", argc, argv[0]);

  if (cli_command_lookup(cli, argv[0], &command)) {
    LOG_ERROR("cli_command_lookup %s", argv[0]);
    return -1;
  } else {
    LOG_DEBUG("cmd=%s", command->command);
  }

  return command->handler(argc, argv, command->ctx);
}

// RX overflow, discard line
static void cli_rx_overflow(struct cli *cli)
{
  LOG_WARN("len=%s", cli->rx_ptr - cli->rx_buf);

  cli->rx_ptr = cli->rx_buf;
}

// uart->rx_buf contains a '\0'-terminated string
static void cli_rx_line(struct cli *cli)
{
  LOG_INFO("%s", cli->rx_buf);

  cli_command(cli, cli->rx_buf);

  cli->rx_ptr = cli->rx_buf;
}

static void cli_rx(struct cli *cli, const struct uart_rx_event *rx)
{
  if (rx->flags & UART_RX_OVERFLOW) {
      cli_rx_overflow(cli);
  }

  for (const char *in = rx->buf; in < rx->buf + rx->len; in++) {
    if (*in == '\r') {
      continue;
    } else if (*in == '\n') {
      *cli->rx_ptr = '\0';

      cli_rx_line(cli);
    } else {
      *cli->rx_ptr++ = *in;
    }

    if (cli->rx_ptr >= cli->rx_buf + sizeof(cli->rx_buf)) {
      cli_rx_overflow(cli);
    }
  }
}

void cli_task(void *arg)
{
  struct cli *cli = arg;
  struct uart_rx_event rx_event;

  LOG_INFO("init cli=%p", cli);

  for (;;) {
    if (xQueueReceive(cli->uart_rx_queue, &rx_event, portMAX_DELAY)) {
      if (rx_event.len < sizeof(rx_event.buf)) {
        rx_event.buf[rx_event.len] = '\0';
      }

      LOG_DEBUG("read {overflow=%d} len=%d: %.32s",
        rx_event.flags & UART_RX_OVERFLOW,
        rx_event.len, rx_event.buf
      );

      cli_rx(cli, &rx_event);

    } else {
      LOG_ERROR("xQueueReceive");
    }

  }
}

int init_cli(struct user_config *config, const struct cli_command *commands)
{
  int err;

  cli.rx_ptr = cli.rx_buf;
  cli.commands = commands;

  if ((cli.uart_rx_queue = xQueueCreate(UART_RX_QUEUE_SIZE, sizeof(struct uart_rx_event))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if ((err = xTaskCreate(&cli_task, (void *) "cli", CLI_TASK_STACK, &cli, tskIDLE_PRIORITY + 2, &cli.task)) <= 0) {
    LOG_ERROR("xTaskCreate cli: %d", err);
    return -1;
  }

  if ((err = uart_start_recv(cli.uart_rx_queue))) {
    LOG_ERROR("uart_start_recv");
    return err;
  }

  return 0;
}
