#include <config.h>
#include "state.h"

#include <freertos/task.h>

const char *config_state_str(enum config_state state)
{
  switch (state) {
    case CONFIG_STATE_INIT:   return "INIT";
    case CONFIG_STATE_LOAD:   return "LOAD";
    case CONFIG_STATE_BOOT:   return "BOOT";
    case CONFIG_STATE_DIRTY:  return "DIRTY";
    case CONFIG_STATE_SAVE:   return "SAVE";
    case CONFIG_STATE_RESET:  return "RESET";
    default:                  return "?";
  }
}

void config_state(struct config *config, enum config_state state)
{
  config->state = state;
  config->tick = xTaskGetTickCount();
}
