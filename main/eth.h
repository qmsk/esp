#pragma once

#include <cmd.h>
#include <config.h>

#include <sdkconfig.h>

#if CONFIG_ETH_ENABLED
  int init_eth();
  int init_eth_netif();
  int init_eth_events();

  int start_eth();

  extern const struct cmdtab eth_cmdtab;
  extern const struct configtab eth_configtab[];
#endif
