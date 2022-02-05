#include "console.h"
#include "config.h"

#include <logging.h>

void app_main(void)
{
  int err;

  LOG_INFO("boot");

  if ((err = init_console())) {
    LOG_ERROR("init_console");
    abort();
  }

  if ((err = init_config())) {
    LOG_WARN("init_config");
    // TODO: alert()
  }

  LOG_INFO("start");

  if ((err = start_console())) {
    LOG_ERROR("start_console");
    abort();
  }
}
