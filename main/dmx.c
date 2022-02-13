#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"

#include <logging.h>

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

int start_dmx()
{
  // TODO
  return 0;
}
