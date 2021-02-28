#include "cli.h"
#include "uart.h"

#include <logging.h>

#include <stdio.h>
#include <stdlib.h>

void app_main()
{
  LOG_INFO("start");

  if (init_uart()) {
    LOG_ERROR("uart_init");
    abort();
  }

  if (init_cli()) {
    LOG_ERROR("init_cli");
    abort();
  }

  LOG_INFO("complete");
}
