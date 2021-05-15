#include "activity_led.h"
#include "artnet.h"
#include "atx_psu.h"
#include "cli.h"
#include "config.h"
#include "dmx.h"
#include "user_led.h"
#include "uart.h"
#include "spi_leds.h"
#include "wifi.h"

#include <logging.h>

#include <stdio.h>
#include <stdlib.h>

void app_main()
{
  LOG_INFO("start");

  if (init_user_led()) {
    LOG_ERROR("init_user_led");
    abort();
  }

  if (init_uart()) {
    LOG_ERROR("uart_init");
    abort();
  }

  if (init_cli()) {
    LOG_ERROR("init_cli");
    abort();
  }

  if (init_config()) {
    LOG_ERROR("init_config");
    abort();
  }

  if (init_activity_led()) {
    LOG_ERROR("init_activity_led");
    abort();
  }

  if (init_atx_psu()) {
    LOG_ERROR("init_atx_psu");
    abort();
  }

  if (init_wifi()) {
    LOG_ERROR("init_wifi");
    abort();
  }

  if (init_artnet()) {
    LOG_ERROR("init_artnet");
    abort();
  }

  if (init_spi_leds()) {
    LOG_ERROR("init_spi_leds");
    abort();
  }

  if (init_dmx()) {
    LOG_ERROR("init_dmx");
    abort();
  }

  LOG_INFO("complete");

  if (start_artnet()) {
    LOG_ERROR("start_artnet");
    abort();
  }
}
