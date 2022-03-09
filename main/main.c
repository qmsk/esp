#include "artnet.h"
#include "atx_psu.h"
#include "console.h"
#include "config.h"
#include "dev_mutex.h"
#include "dmx.h"
#include "leds.h"
#include "log.h"
#include "http.h"
#include "pin_mutex.h"
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

  if ((err = init_pin_mutex())) {
    LOG_ERROR("init_pin_mutex");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = init_dev_mutex())) {
    LOG_ERROR("init_dev_mutex");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

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
  }

  if ((err = init_http())) {
    LOG_ERROR("init_http");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if ((err = init_atx_psu())) {
    LOG_ERROR("init_atx_psu");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if ((err = init_artnet())) {
    LOG_ERROR("init_artnet");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if ((err = init_dmx())) {
    LOG_ERROR("init_dmx");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if ((err = init_leds())) {
    LOG_ERROR("init_leds");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  LOG_INFO("start");

  if ((err = start_console())) {
    LOG_ERROR("start_console");
    user_alert(USER_ALERT_ERROR_START);
  }

  if ((err = start_http())) {
    LOG_ERROR("start_http");
    user_alert(USER_ALERT_ERROR_START);
  }

  if ((err = start_atx_psu())) {
    LOG_ERROR("start_atx_psu");
    user_alert(USER_ALERT_ERROR_START);
  }

  if ((err = start_dmx())) {
    LOG_ERROR("start_dmx");
    user_alert(USER_ALERT_ERROR_START);
  }

  if ((err = start_leds())) {
    LOG_ERROR("start_leds");
    user_alert(USER_ALERT_ERROR_START);
  }

  // after inputs/outputs added
  if ((err = start_artnet())) {
    LOG_ERROR("start_artnet");
    user_alert(USER_ALERT_ERROR_START);
  }

  LOG_INFO("fini");
}
