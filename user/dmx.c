#include "dmx.h"
#include "dmx_cmd.h"
#include "dmx_config.h"
#include "artnet_dmx.h"

#include <lib/logging.h>
#include <lib/uart.h>
#include <drivers/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <lib/cmd.h>
#include <stdlib.h>
#include <string.h>

#define DMX_BAUD 250000
#define DMX_BREAK_US 128 // 32 bits
#define DMX_MARK_US 12 // 3 bits
#define DMX_SIZE 512
#define DMX_TASK_STACK 512

struct uart *dmx_uart = &uart1;

struct dmx {
  struct uart *uart;

  xQueueHandle artnet_queue;
  xTaskHandle artnet_task;
  struct artnet_dmx artnet_dmx;
} dmx;

const UART_Config dmx_uart_config = {
  .baud_rate  = DMX_BAUD,
  .data_bits  = UART_WordLength_8b,
  .parity     = UART_Parity_None,
  .stop_bits  = UART_StopBits_2,
};

int dmx_init(struct dmx *dmx)
{
  return 0;
}

// Write DMX packet: break, start code frame, data frames
int dmx_send(struct dmx *dmx, enum dmx_cmd cmd, void *data, size_t len)
{
  int err;

  if ((err = uart_break(dmx->uart, DMX_BREAK_US, DMX_MARK_US)))
    return err;

  if ((err = uart_putc(dmx->uart, cmd)) < 0)
    return err;

  if ((err = uart_write(dmx->uart, data, len)) < 0)
    return err;

  return 0;
}

void dmx_artnet_task(void *arg)
{
  struct dmx *dmx = arg;
  int err;

  LOG_DEBUG("dmx=%p", dmx);

  for (;;) {
    if (!xQueueReceive(dmx->artnet_queue, &dmx->artnet_dmx, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
    } else if ((err = dmx_send(dmx, DMX_CMD_DIMMER, dmx->artnet_dmx.data, dmx->artnet_dmx.len))) {
      LOG_WARN("dmx_send");
    } else {
      LOG_DEBUG("dmx send len=%u", dmx->artnet_dmx.len);
    }
  }
}

int dmx_setup(struct dmx *dmx, struct dmx_config *config)
{
  struct uart *uart = &uart1;
  int err;

  if (GPIO_Exists(config->gpio)) {
    GPIO_SetupOutput(config->gpio, GPIO_OUTPUT_LOW);
  } else {
    LOG_ERROR("invalid gpio=%u", config->gpio);
    return -1;
  }

  if ((err = uart_setup(uart, &dmx_uart_config))) {
    LOG_ERROR("uart_setup");
    return err;
  }

  dmx->uart = uart;

  if (config->artnet_universe) {
    if ((dmx->artnet_queue = xQueueCreate(1, sizeof(struct artnet_dmx))) == NULL) {
      LOG_ERROR("xQueueCreate");
      return -1;
    }

    if ((err = start_artnet_output(config->artnet_universe, dmx->artnet_queue))) {
      LOG_ERROR("patch_artnet_output");
      return err;
    }
  }

  LOG_INFO("output enable: gpio=%u", config->gpio);

  // enable TX
  GPIO_OutputHigh(config->gpio);

  return 0;
}

int init_dmx(struct dmx_config *config)
{
  int err;

  if ((err = dmx_init(&dmx))) {
    return err;
  }

  if ((err = dmx_setup(&dmx, config))) {
    return err;
  }

  if (!dmx.artnet_queue) {

  } else if ((err = xTaskCreate(&dmx_artnet_task, (signed char *) "dmx-artnet", DMX_TASK_STACK, &dmx, tskIDLE_PRIORITY + 2, &dmx.artnet_task)) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("dmx-artnet task=%p", dmx.artnet_task);
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

  if ((err = dmx_send(&dmx, DMX_CMD_DIMMER, buf, count)))
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

  if ((err = dmx_send(&dmx, DMX_CMD_DIMMER, buf, count)))
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
      goto error;
  }

  if ((err = dmx_send(&dmx, DMX_CMD_DIMMER, buf, count)))
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
