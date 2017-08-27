#include "dmx.h"
#include "uart.h"
#include "logging.h"

#include <cmd.h>
#include <stdlib.h>
#include <string.h>

#define DMX_BAUD 250000
#define DMX_BREAK_US 128 // 32 bits
#define DMX_MARK_US 12 // 3 bits
#define DMX_SIZE 512

struct uart *dmx_uart = &uart1;

const UART_Config dmx_uart_config = {
  .baud_rate  = DMX_BAUD,
  .data_bits  = UART_WordLength_8b,
  .parity     = UART_Parity_None,
  .stop_bits  = UART_StopBits_2,
};

// Write DMX packet: break, start code frame, data frames
int dmx_send(enum dmx_cmd cmd, void *data, size_t len)
{
  int err;

  if ((err = uart_break(dmx_uart, DMX_BREAK_US, DMX_MARK_US)))
    return err;

  if ((err = uart_putc(dmx_uart, cmd)) < 0)
    return err;

  if ((err = uart_write(dmx_uart, data, len)) < 0)
    return err;

  return 0;
}

int init_dmx(struct user_config *config)
{
  int err;

  if ((err = uart_setup(dmx_uart, &dmx_uart_config))) {
    LOG_ERROR("uart_setup");
    return err;
  }

  return 0;
}

int dmx_cmd_zero(int argc, char **argv, void *ctx)
{
  unsigned count;
  uint8_t *buf;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &count)))
    return err;

  if ((buf = malloc(count)) == NULL) {
    LOG_ERROR("malloc");
    return -CMD_ERR;
  }

  memset(buf, 0, count);

  if ((err = dmx_send(DMX_CMD_DIMMER, buf, count)))
    goto error;

error:
  free(buf);

  return err;
}

int dmx_cmd_all(int argc, char **argv, void *ctx)
{
  unsigned count;
  unsigned value;
  uint8_t *buf;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &count)))
    return err;

  if ((err = cmd_arg_uint(argc, argv, 2, &value)))
    return err;

  if ((buf = malloc(count)) == NULL) {
    LOG_ERROR("malloc");
    return -CMD_ERR;
  }

  memset(buf, value, count);

  if ((err = dmx_send(DMX_CMD_DIMMER, buf, count)))
    goto error;

error:
  free(buf);

  return err;
}

int dmx_cmd_values(int argc, char **argv, void *ctx)
{
  unsigned count = argc - 1;
  uint8_t *buf;
  int err;

  if ((buf = malloc(count)) == NULL) {
    LOG_ERROR("malloc");
    return -CMD_ERR;
  }

  for (unsigned i = 0; i < count; i++) {
    if ((err = cmd_arg_uint8(argc, argv, 1 + i, &buf[i])))
      return err;
  }

  if ((err = dmx_send(DMX_CMD_DIMMER, buf, count)))
    goto error;

error:
  free(buf);

  return err;
}

const struct cmd dmx_commands[] = {
  { "zero",   dmx_cmd_zero,   .usage = "COUNT", .describe = "Send COUNT channels at zero" },
  { "all",    dmx_cmd_all,    .usage = "COUNT VALUE", .describe = "Send COUNT channel at VALUE" },
  { "values", dmx_cmd_values, .usage = "[VALUE [...]]", .describe = "Send channels from VALUE..." },
  {}
};

const struct cmdtab dmx_cmdtab = {
  .commands = dmx_commands,
};
