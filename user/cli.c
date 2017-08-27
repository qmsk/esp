#include "cli.h"
#include "uart.h"
#include "logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <cmd.h>

#define CLI_TX_BUF 512
#define CLI_RX_BUF 512
#define CLI_TASK_STACK 512
#define UART_RX_QUEUE_SIZE 8

struct cli {
  xTaskHandle task;
  xQueueHandle uart_rx_queue;

  char tx_buf[CLI_TX_BUF];
  char rx_buf[CLI_RX_BUF], *rx_ptr;

  const struct cmdtab *commands;
} cli;

/**
 * @return 0 on success
 */
void cli_putc(char c)
{
  uart_putc(&uart0, c);
}

/**
 * @return
 */
void cli_printf(const char *fmt, ...)
{
  va_list vargs;
  char *buf = cli.tx_buf;
  char *ptr = cli.tx_buf;
  char *end = cli.tx_buf + sizeof(cli.tx_buf);
  int ret;

  va_start(vargs, fmt);
  ret = vsnprintf(ptr, end - ptr, fmt, vargs);
  va_end(vargs);

  if (ret > sizeof(cli.tx_buf)) {
    LOG_WARN("truncate ret=%d: fmt=%s", ret, fmt);
    ptr = end;
  } else {
    ptr += ret;
  }

  uart_write(&uart0, buf, ptr - buf);
}

void cli_ctx_print(const struct cmdctx *ctx)
{
  if (ctx->parent) {
    cli_ctx_print(ctx->parent);
  }

  if (ctx->cmd) {
    cli_printf(" %s", ctx->cmd->name);
  }
}

int cli_cmd_help(const struct cmdtab *cmdtab, const struct cmdctx *parent)
{
  struct cmdctx ctx = {
    .parent = parent,
    .cmdtab = cmdtab,
  };

  for (const struct cmd *cmd = cmdtab->commands; cmd->name; cmd++) {
    ctx.cmd = cmd;

    if (cmd->func) {
      cli_ctx_print(&ctx);

      if (cmd->usage) {
        cli_printf(" %s", cmd->usage);
      }

      if (cmd->describe) {
        cli_printf(": %s\n", cmd->describe);
      } else {
         cli_printf("\n");
       }
    }

    if (cmd->subcommands) {
      cli_cmd_help(cmd->subcommands, &ctx);
    }
  }

  return 0;
}

int cli_cmd_error(const struct cmdctx *ctx, enum cmd_error err, const char *arg)
{
  cli_printf("!");
  if (ctx->cmd || ctx->parent) {
    cli_ctx_print(ctx);
    cli_printf(": ");
  } else {
    cli_printf(" ");
  }
  cli_printf("%s", cmd_strerror(err));
  if (err == CMD_ERR_USAGE && ctx->cmd && ctx->cmd->usage)
    cli_printf(": %s\n", ctx->cmd->usage);
  else if (arg)
    cli_printf(": %s\n", arg);
  else
    cli_printf("\n");

  if (err == CMD_ERR_NOT_FOUND || err == CMD_ERR_MISSING_SUBCOMMAND) {
    cli_cmd_help(ctx->cmdtab, ctx->parent);
  }

  return 0;
}

int cli_help_cmd(int argc, char **argv, void *ctx)
{
  return cli_cmd_help(cli.commands, NULL);
}

static void cli_cmd(struct cli *cli, char *line)
{
  int err;

  if ((err = cmd_eval(cli->commands, line)) < 0) {
    LOG_ERROR("cmd_eval %s: %s", line, cmd_strerror(-err));
  } else if (err) {
    LOG_ERROR("cmd %s: %d", line, err);
  }
}

//
static void cli_rx_start(struct cli *cli)
{
  cli->rx_ptr = cli->rx_buf;

  cli_printf("> ");
}

// RX overflow, discard line
static void cli_rx_overflow(struct cli *cli)
{
  LOG_WARN("len=%s", cli->rx_ptr - cli->rx_buf);

  cli_rx_start(cli);
}

// uart->rx_buf contains a '\0'-terminated string
static void cli_rx_line(struct cli *cli)
{
  LOG_DEBUG("%s", cli->rx_buf);

  cli_cmd(cli, cli->rx_buf);

  cli_rx_start(cli);
}

static void cli_rx(struct cli *cli, const struct uart_event *rx)
{
  if (rx->flags & UART_RX_OVERFLOW) {
      cli_rx_overflow(cli);
  }

  for (const uint8_t *in = rx->buf; in < rx->buf + rx->len; in++) {
    #ifdef DEBUG
      if (isprint(*in)) {
        LOG_DEBUG("%c", *in);
      } else {
        LOG_DEBUG("%#02x", *in);
      }
    #endif

    cli_putc(*in);

    if (*in == '\r') {
      continue;
    } else if (*in == '\b' && cli->rx_ptr > cli->rx_buf) {
      cli->rx_ptr--;

      cli_printf(" \b");

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
  struct uart_event rx_event;

  LOG_INFO("init cli=%p", cli);

  cli_rx_start(cli);

  for (;;) {
    if (xQueueReceive(cli->uart_rx_queue, &rx_event, portMAX_DELAY)) {
      LOG_DEBUG("rx overflow=%d len=%d", rx_event.flags & UART_RX_OVERFLOW, rx_event.len);

      cli_rx(cli, &rx_event);

    } else {
      LOG_ERROR("xQueueReceive");
    }
  }
}

int init_cli(struct user_config *config, const struct cmdtab *commands)
{
  int err;

  cli.rx_ptr = cli.rx_buf;
  cli.commands = commands;

  if ((cli.uart_rx_queue = xQueueCreate(UART_RX_QUEUE_SIZE, sizeof(struct uart_event))) == NULL) {
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

  LOG_INFO("");

  return 0;
}
