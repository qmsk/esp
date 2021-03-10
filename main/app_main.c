#include "apa102.h"
#include "cli.h"
#include "config.h"
#include "user_led.h"
#include "uart.h"
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

  if (init_wifi()) {
    LOG_ERROR("init_wifi");
    abort();
  }

  if (init_apa102()) {
    LOG_ERROR("init_apa102");
    abort();
  }

  LOG_INFO("complete");
}
