#include <dmx_uart.h>
#include "dmx.h"
#include "console.h"
#include "pin_mutex.h"

#include <logging.h>
#include <uart.h>

// fit one complete DMX frame into the uart RX/TX buffers
#define DMX_UART_RX_BUFFER_SIZE (512 + 1)
#define DMX_UART_TX_BUFFER_SIZE (512 + 1)

#define DMX_UART_FRAME_TIMEOUT 20

// used for UART setup + DMX input
#define DMX_TASK_STACK 2048
#define DMX_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

xTaskHandle dmx_task;
struct uart *dmx_uart;
SemaphoreHandle_t dmx_uart_dev_mutex, dmx_uart_pin_mutex;

int init_dmx_uart(const struct dmx_config *config, bool inputs_enabled, bool outputs_enabled)
{
  enum uart_port uart_port = config->uart;
  size_t rx_buffer_size = inputs_enabled ? DMX_UART_RX_BUFFER_SIZE : 0;
  size_t tx_buffer_size = outputs_enabled ? DMX_UART_TX_BUFFER_SIZE : 0;
  int err;

  switch(config->uart) {
    case UART_0:
      // avoid USB UART console conflict
      uart_port |= UART_0_SWAP;

      if (!inputs_enabled) {
        uart_port |= UART_TXONLY_BIT;
      }
      if (!outputs_enabled) {
        uart_port |= UART_RXONLY_BIT;
      }

      // note that this does NOT need to acquire PIN_MUTEX_U0RXD, as we are using the swapped RTS/CTS pins
      dmx_uart_dev_mutex = dev_mutex[DEV_MUTEX_UART0];
      dmx_uart_pin_mutex = NULL;
      break;

    case UART_1:
      // TODO
      dmx_uart_dev_mutex = NULL;
      dmx_uart_pin_mutex = NULL;
      break;

    default:
      LOG_ERROR("invalid uart=%d", config->uart);
      return -1;
  }

  LOG_INFO("uart_port=%x, rx_buffer_size=%u, tx_buffer_size=%u",
    uart_port,
    rx_buffer_size,
    tx_buffer_size
  );

  if ((err = uart_new(&dmx_uart, uart_port, rx_buffer_size, tx_buffer_size))) {
    LOG_ERROR("uart_new");
    return err;
  }

  return 0;
}

static bool dmx_outputs_enabled()
{
  for (int i = 0; i < DMX_OUTPUT_COUNT; i++)
  {
    const struct dmx_output_config *config = &dmx_output_configs[i];

    if (config->enabled) {
      return true;
    }
  }

  return false;
}

unsigned count_dmx_artnet_inputs()
{
  const struct dmx_config *config = &dmx_config;

  return config->input_enabled ? 1 : 0;
}

int init_dmx()
{
  const struct dmx_config *config = &dmx_config;
  bool inputs_enabled = config->input_enabled;
  bool outputs_enabled = dmx_outputs_enabled();
  int err;

  if (!config->enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = init_dmx_uart(config, inputs_enabled, outputs_enabled))) {
    LOG_ERROR("init_dmx_uart");
    return err;
  }

  if (inputs_enabled) {
    if ((err = init_dmx_inputs())) {
      LOG_ERROR("init_dmx_inputs");
      return err;
    }
  }

  if (outputs_enabled) {
    if ((err = init_dmx_outputs())) {
      LOG_ERROR("init_dmx_outputs");
      return err;
    }
  }

  return 0;
}

static int start_dmx_uart()
{
  struct uart_options options = dmx_uart_options;
  int err;

  LOG_INFO("clock_div=%u, data_bits=%x parity=%x stop_bits=%x",
    options.clock_div,
    options.data_bits,
    options.parity_bits,
    options.stop_bits
  );

  options.rx_timeout = DMX_UART_FRAME_TIMEOUT;

  // this will block on the dev mutex if the console is running
  options.dev_mutex = dmx_uart_dev_mutex;
  options.pin_mutex = dmx_uart_pin_mutex;

  if (!dmx_uart_dev_mutex) {
    LOG_INFO("UART dev_mutex is not set, assume UART is available");
  } else if (uxSemaphoreGetCount(dmx_uart_dev_mutex) > 0) {
    LOG_INFO("UART dev_mutex=%p is available", dmx_uart_dev_mutex);
  } else if (is_console_running()) {
    LOG_WARN("UART dev_mutex=%p will wait for console to exit...", dmx_uart_dev_mutex);
  } else {
    LOG_WARN("DMX dev_mutex=%p will wait for UART to become available...", dmx_uart_dev_mutex);
  }

  if ((err = uart_setup(dmx_uart, options))) {
    LOG_ERROR("uart_setup");
    return err;
  }

  LOG_INFO("UART setup complete");

  return 0;
}

static void stop_dmx_uart()
{
  LOG_INFO("DMX UART stopping...");

  if (uart_teardown(dmx_uart)) {
    LOG_ERROR("uart_teardown");
  } else {
    LOG_INFO("DMX UART stopped");
  };
}

static void dmx_main(void *ctx)
{
  int err;

  // XXX: is the task delay racy? We want to allow any other UART user to acquire the dev lock
  for (;; vTaskDelay(1)) {
    // use a separate task for UART setup, because it may block if the UART is busy
    if ((err = start_dmx_uart())) {
      LOG_ERROR("start_dmx_uart");
      continue;
    }

    if (dmx_input_state && dmx_input_state->dmx_input) {
      if ((err = dmx_input_main(dmx_input_state))) {
        LOG_ERROR("dmx_input_main");
      }
    } else {
      LOG_INFO("wait for stop signal...");

      if (!ulTaskNotifyTake(true, portMAX_DELAY)) {
        LOG_ERROR("ulTaskNotifyTake");
      }
    }

    stop_dmx_uart();
  }

  LOG_INFO("stopped task=%p", dmx_task);
  dmx_task = NULL;
  vTaskDelete(NULL);
}

int start_dmx()
{
  const struct dmx_config *config = &dmx_config;
  int err;

  if (!config->enabled) {
    return 0;
  }

  if (!dmx_uart) {
    LOG_ERROR("DMX UART is missing, not starting");
    return -1;
  }

  LOG_INFO("start");

  if (xTaskCreate(&dmx_main, "dmx", DMX_TASK_STACK, NULL, DMX_TASK_PRIORITY, &dmx_task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("task=%p", dmx_task);
  }

  if ((err = start_dmx_outputs())) {
    LOG_ERROR("start_dmx_outputs");
    return err;
  }

  return 0;
}

int restart_dmx()
{
  const struct dmx_config *config = &dmx_config;
  int err;

  if (!config->enabled) {
    return 0;
  }

  if (!dmx_task) {
    return 0;
  }

  if (dmx_input_state && dmx_input_state->dmx_input) {
    if ((err = stop_dmx_input(dmx_input_state))) {
      LOG_ERROR("stop_dmx_input");
      return err;
    }
  } else {
    LOG_INFO("notify task=%p to restart", dmx_task);

    xTaskNotifyGive(dmx_task);
  }

  return 0;
}
