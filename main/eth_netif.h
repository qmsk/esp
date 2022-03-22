#pragma once

#include <sdkconfig.h>

#if CONFIG_ETH_ENABLED
  int set_eth_netif_hostname(const char *hostname);
  int set_eth_netif_ip(const char *ip, const char *netmask, const char *gw);

  int set_eth_netif_static();
  int set_eth_netif_dhcpc();
  int set_eth_netif_dhcps();

  int is_eth_netif_dhcpc();
#endif
