#include <dmx_input.h>
#include "dmx.h"

#include <logging.h>
#include <uart0.h>

struct uart0 *dmx_uart0;
struct dmx_input *dmx_input;
xTaskHandle dmx_input_task;

// fit one complete DMX frame into the uart1 RX buffer
#define DMX_RX_BUFFER_SIZE (512 + 1)

#define DMX_INPUT_FRAME_TIMEOUT 20

#define DMX_INPUT_TASK_STACK 2048
#define DMX_INPUT_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static void dmx_input_main(void *ctx)
{
  int err;

  if ((err = dmx_input_open(dmx_input, dmx_uart0))) {
    LOG_ERROR("dmx_input_open");
    return; // XXX: alert
  }

  for (;;) {
    if ((err = dmx_input_read(dmx_input))) {
      LOG_ERROR("dmx_input_read");
      continue;
    }

    // TODO: artnet input?
  }
}

int init_dmx_input()
{
  const struct dmx_input_config *config = &dmx_input_config;
  struct dmx_input_options options = {
    .frame_timeout = DMX_INPUT_FRAME_TIMEOUT,
  };
  int err;

  if (!config->enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = uart0_new(&dmx_uart0, DMX_RX_BUFFER_SIZE))) {
    LOG_ERROR("uart0_new");
    return err;
  }

  if ((err = dmx_input_new(&dmx_input, options))) {
    LOG_ERROR("dmx_input_new");
    return err;
  }

  if (xTaskCreate(&dmx_input_main, "dmx-input", DMX_INPUT_TASK_STACK, NULL, DMX_INPUT_TASK_PRIORITY, &dmx_input_task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("task=%p", dmx_input_task);
  }

  return 0;
}
