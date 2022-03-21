#pragma once

#include <sdkconfig.h>

#if CONFIG_ETH_ENABLED
  #include <hal/eth_types.h>

  const char *eth_link_str(eth_link_t link);
  const char *eth_speed_str(eth_speed_t speed);
  const char *eth_duplex_str(eth_duplex_t duplex);
#endif
