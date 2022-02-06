#include "console.h"
#include "config.h"
#include "log.h"
#include "status_leds.h"
#include "system.h"
#include "user.h"
#include "wifi.h"

#include <logging.h>
#include <system.h>

void app_main(void)
{
  int err;

  // heap usage is likely to be lowest at app_main() start
  system_update_maximum_free_heap_size();

  if ((err = init_log())) {
    LOG_ERROR("init_log");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  LOG_INFO("boot");

  if ((err = init_status_leds())) {
    LOG_ERROR("init_status_leds");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = init_system())) {
    LOG_ERROR("init_system");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = init_console())) {
    LOG_ERROR("init_console");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = start_status_leds())) {
    LOG_ERROR("start_status_leds");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  LOG_INFO("config");

  if ((err = init_config()) < 0) {
    LOG_ERROR("init_config");
    user_alert(USER_ALERT_ERROR_BOOT);
  } else if (err > 0) {
    LOG_WARN("init_config: not configured");
    user_alert(USER_ALERT_ERROR_CONFIG);
  }

  LOG_INFO("setup");

  if ((err = init_wifi())) {
    LOG_ERROR("init_wifi");
    user_alert(USER_ALERT_ERROR_SETUP);
    abort();
  }

  LOG_INFO("start");

  if ((err = start_console())) {
    LOG_ERROR("start_console");
    abort();
  }

  LOG_INFO("fini");
}
