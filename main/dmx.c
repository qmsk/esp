#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"
#include "user.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// used for UART setup + DMX input
#define DMX_TASK_STACK 2048
#define DMX_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

xTaskHandle dmx_task;

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

  // use a separate task for UART setup, because it may block if the UART is busy
  if ((err = start_dmx_uart())) {
    LOG_ERROR("start_dmx_uart");
    goto error;
  }

  // holds
  if (!dmx_input_state.dmx_input) {
    LOG_INFO("wait...");

    if (!ulTaskNotifyTake(true, portMAX_DELAY)) {
      LOG_ERROR("ulTaskNotifyTake");
      goto error;
    }
  } else if ((err = run_dmx_input(&dmx_input_state))) {
    LOG_ERROR("dmx-input: run_dmx_input");
    goto error;
  }

error:
  // TODO: stop uart

  user_alert(USER_ALERT_ERROR_DMX);
  LOG_ERROR("task=%p stopped", dmx_task);
  dmx_task = NULL;
  vTaskDelete(NULL);
}

int start_dmx()
{
  bool enabled;
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

  if (xTaskCreate(&dmx_main, "dmx", DMX_TASK_STACK, NULL, DMX_TASK_PRIORITY, &dmx_task) <= 0) {
    LOG_ERROR("xTaskCreate");
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

// TODO: stop_dmx()
