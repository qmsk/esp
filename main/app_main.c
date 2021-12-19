#include "artnet_init.h"
#include "atx_psu.h"
#include "config.h"
#include "console.h"
#include "dmx_init.h"
#include "http.h"
#include "log.h"
#include "mdns.h"
#include "spi_leds_init.h"
#include "status_leds.h"
#include "system.h"
#include "wifi_init.h"

#include <logging.h>
#include <system.h>

#include <stdio.h>
#include <stdlib.h>

void app_main()
{
  int err;

  // heap usage is likely to be lowest at app_main() start
  system_update_maximum_free_heap_size();

  if (init_log()) {
    LOG_ERROR("init_log");
    user_alert(USER_ALERT_ERROR_BOOT); // TODO: early gpio alert output before init_status_leds()?
    abort();
  }

  // system-level init stage, abort on failures
  LOG_INFO("init");

  if (init_status_leds()) {
    LOG_ERROR("init_status_leds");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if (init_system()) {
    LOG_ERROR("init_system");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if (init_console()) {
    LOG_ERROR("init_console");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  // config stage, terminate on failure
  LOG_INFO("config");

  if ((err = init_config()) < 0) {
    LOG_ERROR("init_config");
    user_alert(USER_ALERT_ERROR_BOOT);
  } else if (err > 0) {
    LOG_WARN("init_config: not configured");
    user_alert(USER_ALERT_ERROR_CONFIG);
  }

  if (start_console()) {
    LOG_ERROR("start_console");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  // setup stage, continue on failure
  LOG_INFO("setup");

  // setup stage, continue on failure
  if (init_atx_psu()) {
    LOG_ERROR("init_atx_psu");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if (init_wifi()) {
    LOG_ERROR("init_wifi");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if (init_mdns()) {
    LOG_ERROR("init_mdns");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if (init_http()) {
    LOG_ERROR("init_http");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if (init_artnet()) {
    LOG_ERROR("init_artnet");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if (init_spi_leds()) {
    LOG_ERROR("init_spi_leds");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if (init_dmx_input()) {
    LOG_ERROR("init_dmx_input");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if (init_dmx_outputs()) {
    LOG_ERROR("init_dmx_outputs");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  LOG_INFO("start");

  if (start_artnet()) {
    LOG_ERROR("start_artnet");
    user_alert(USER_ALERT_ERROR_SETUP);
  }
}
