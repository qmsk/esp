#pragma once

#include "sdkconfig.h"

#if CONFIG_ESP_CONSOLE_UART_NUM == 0
# define CLI_ENABLED 1
#endif

#if CLI_ENABLED

int init_cli();

#endif
