#pragma once

#include <sdkconfig.h>

#if CONFIG_ETH_PHY_LAN87XX
# define ETH_ENABLED 1
#else
# define ETH_ENABLED 0
#endif

enum eth_mode {
  ETH_MODE_NONE,
  ETH_MODE_STATIC,
  ETH_MODE_DHCP_CLIENT,
  ETH_MODE_DHCP_SERVER,
};

struct eth_config {
  bool enabled;
  char hostname[32];

  int mode;

  char ip[16];
  char netmask[16];
  char gw[16];
};

bool is_eth_enabled();
bool is_eth_dhcp_client();
bool is_eth_dhcp_server();

int config_eth();
