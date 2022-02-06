#include "console.h"
#include "config.h"
#include "log.h"
#include "system.h"

#include <logging.h>
#include <system.h>

void app_main(void)
{
  int err;

  // heap usage is likely to be lowest at app_main() start
  system_update_maximum_free_heap_size();

  if ((err = init_log())) {
    LOG_ERROR("init_log");
    abort();
  }

  LOG_INFO("boot");

  if ((err = init_console())) {
    LOG_ERROR("init_console");
    abort();
  }

  if ((err = init_config())) {
    LOG_WARN("init_config");
    // TODO: alert()
  }

  if ((err = init_system())) {
    LOG_ERROR("init_system");
    abort();
  }

  LOG_INFO("start");

  if ((err = start_console())) {
    LOG_ERROR("start_console");
    abort();
  }

  LOG_INFO("fini");
}
