#include "artnet_init.h"
#include "atx_psu.h"
#include "cli.h"
#include "config.h"
#include "dmx_init.h"
#include "http.h"
#include "mdns.h"
#include "uart.h"
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

  // system-level boot stage, abort on failures
  LOG_INFO("boot");

  // XXX: driver/uart will set U0TXD pin mux to U0TXD despite using bootloader CONFIG_ESP_UART0_SWAP_IO,
  //      which breaks status_leds GPIO1. Initialize this first.
  if (init_uart()) {
    LOG_ERROR("uart_init");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

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

#if CLI_ENABLED
  if (init_cli()) {
    LOG_ERROR("init_cli");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }
#endif

  // config stage, terminate on failure
  LOG_INFO("boot");

  if ((err = init_config()) < 0) {
    LOG_ERROR("init_config");
    user_alert(USER_ALERT_ERROR_SETUP);
  } else if (err > 0) {
    LOG_WARN("init_config: not configured");
    user_alert(USER_ALERT_ERROR_CONFIG);
  }

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

  LOG_INFO("complete");

  if (start_artnet()) {
    LOG_ERROR("start_artnet");
    user_alert(USER_ALERT_ERROR_SETUP);
  }
}
