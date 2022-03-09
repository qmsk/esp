#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"
#include "user.h"
#include "tasks.h"

#include <logging.h>

#define DMX_TASK_RESTART_DELAY (10)

xTaskHandle dmx_task;

unsigned count_dmx_artnet_inputs()
{
  if (dmx_input_config.enabled) {
    return 1;
  } else {
    return 0;
  }
}

int init_dmx()
{
  int err;

  if ((err = init_dmx_uart())) {
    LOG_ERROR("init_dmx_uart");
    return err;
  }

  if ((err = init_dmx_gpio())) {
    LOG_ERROR("init_dmx_gpio");
    return err;
  }

  if ((err = init_dmx_input())) {
    LOG_ERROR("init_dmx_input");
    return err;
  }

  if ((err = init_dmx_outputs())) {
    LOG_ERROR("init_dmx_outputs");
    return err;
  }

  return 0;
}

void dmx_main()
{
  int err;

  // delay uart setup on restart, we want to allow any other UART user to acquire the dev lock
  // XXX: racy?
  for (;; vTaskDelay(DMX_TASK_RESTART_DELAY)) {
    // use a separate task for UART setup, because it may block if the UART is busy
    if ((err = start_dmx_uart())) {
      LOG_ERROR("start_dmx_uart");
      goto error;
    }

    if (!dmx_input_state.dmx_input) {
      LOG_INFO("wait for restart signal...");

      // allow outputs to run until restarted
      if (!ulTaskNotifyTake(true, portMAX_DELAY)) {
        LOG_ERROR("ulTaskNotifyTake");
        goto error;
      }
    } else if ((err = run_dmx_input(&dmx_input_state))) {
      LOG_ERROR("dmx-input: run_dmx_input");
      goto error;
    }

    LOG_INFO("stopping DMX UART...");

    stop_dmx_uart();
  }

error:
  user_alert(USER_ALERT_ERROR_DMX);
  LOG_ERROR("task=%p stopped", dmx_task);
  dmx_task = NULL;
  vTaskDelete(NULL);
}

int start_dmx()
{
  struct task_options task_options = {
    .main       = dmx_main,
    .name       = DMX_TASK_NAME,
    .stack_size = DMX_TASK_STACK,
    .arg        = NULL,
    .priority   = DMX_TASK_PRIORITY,
    .handle     = &dmx_task,
    .affinity   = DMX_TASK_AFFINITY,
  };
  bool enabled = false;
  int err;

  if (dmx_input_config.enabled && dmx_input_state.dmx_input) {
    enabled = true;
  }

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++) {
    if (dmx_output_configs[i].enabled && dmx_output_states[i].dmx_output) {
      enabled = true;
    }
  }

  if (!enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  LOG_INFO("start");

  if (start_task(task_options)) {
    LOG_ERROR("start_task");
    return -1;
  } else {
    LOG_DEBUG("task=%p", dmx_task);
  }

  // these can be started before the uart is setup, they will just fail to output until then
  if ((err = start_dmx_outputs())) {
    LOG_ERROR("dmx-start_dmx_outputs");
    return err;
  }

  return 0;
}

void release_dmx_uart0()
{
  if (!dmx_task) {
    LOG_DEBUG("uart task not running");
    return;
  }

  if (!query_dmx_uart0()) {
    LOG_DEBUG("uart0 not in use");
    return;
  }

  LOG_WARN("interrupting DMX task to release UART0");

  if (dmx_input_state.dmx_input) {
    LOG_INFO("notify dmx_input to stop reading");

    // XXX: race with any uart_setup() -> dmx_input_open()
    if (stop_dmx_input(&dmx_input_state)) {
      LOG_WARN("stop_dmx_input");
    }
  } else  {
    LOG_INFO("notify task=%p to restart", dmx_task);

    xTaskNotifyGive(dmx_task);
  }
}
