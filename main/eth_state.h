#pragma once

#include <sdkconfig.h>

#if CONFIG_ETH_ENABLED
  #include <esp_eth.h>

  esp_eth_handle_t get_eth_handle();
#endif
