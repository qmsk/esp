#include "eth.h"
#include "eth_config.h"
#include "eth_state.h"
#include "eth_netif.h"

#include <logging.h>

#if CONFIG_ETH_ENABLED
  #include <esp_eth.h>
  #include <esp_err.h>

  #include <string.h>

  struct eth_config eth_config = {};

  #define ETH_HOSTNAME_FMT "qmsk-esp-%02x%02x%02x"
  #define ETH_HOSTNAME_MAX_SIZE 32

  #define ETH_CONFIG_MODE_DEFAULT ETH_MODE_DHCP_CLIENT


  const struct config_enum eth_mode_enum[] = {
    { "NONE",         ETH_MODE_NONE         },
    { "STATIC",       ETH_MODE_STATIC       },
    { "DHCP_CLIENT",  ETH_MODE_DHCP_CLIENT  },
    { "DHCP_SERVER",  ETH_MODE_DHCP_SERVER  },
    {}
  };

  const struct configtab eth_configtab[] = {
    { CONFIG_TYPE_BOOL, "enabled",
      .description = (
        "Setup the Ethernet interface."
      ),
      .bool_type = { .value = &eth_config.enabled },
    },
    { CONFIG_TYPE_STRING, "hostname",
      .string_type = { .value = eth_config.hostname, .size = sizeof(eth_config.hostname) },
    },
    { CONFIG_TYPE_ENUM, "mode",
      .enum_type = { .value = &eth_config.mode, .values = eth_mode_enum, .default_value = ETH_CONFIG_MODE_DEFAULT },
    },
    { CONFIG_TYPE_STRING, "ip",
      .string_type = { .value = eth_config.ip, .size = sizeof(eth_config.ip) },
    },
    { CONFIG_TYPE_STRING, "netmask",
      .string_type = { .value = eth_config.netmask, .size = sizeof(eth_config.netmask) },
    },
    { CONFIG_TYPE_STRING, "gw",
      .string_type = { .value = eth_config.gw, .size = sizeof(eth_config.gw) },
    },
    {}
  };

  bool is_eth_enabled()
  {
    return eth_config.enabled;
  }

  bool is_eth_dhcp_client()
  {
    return eth_config.mode == ETH_MODE_DHCP_CLIENT;
  }

  bool is_eth_dhcp_server()
  {
    return eth_config.mode == ETH_MODE_DHCP_SERVER;
  }

  static int config_eth_hostname(const struct eth_config *config)
  {
    esp_eth_handle_t eth_handle = get_eth_handle();
    char hostname[ETH_HOSTNAME_MAX_SIZE];
    uint8_t mac[6];
    bool use_config_hostname;
    esp_err_t err;

    if (strlen(config->hostname) >= sizeof(hostname)) {
      LOG_WARN("Invalid hostname with len=%d > max=%u", strlen(config->hostname), sizeof(hostname));

      use_config_hostname = false;
    } else if (strlen(config->hostname) > 0) {
      use_config_hostname = true;
    } else {
      use_config_hostname = false;
    }

    if (use_config_hostname) {
      LOG_INFO("Using config hostname: %s", config->hostname);

      strlcpy(hostname, config->hostname, sizeof(hostname));

    } else if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac))) {
      LOG_ERROR("esp_eth_ioctl ETH_CMD_G_MAC_ADDR: %s", esp_err_to_name(err));

      snprintf(hostname, sizeof(hostname), ETH_HOSTNAME_FMT, 0x00, 0x00, 0x00);

    } else {
      LOG_INFO("Using ETH default hostname: " ETH_HOSTNAME_FMT, mac[3], mac[4], mac[5]);

      snprintf(hostname, sizeof(hostname), ETH_HOSTNAME_FMT, mac[3], mac[4], mac[5]);
    }

    if ((err = set_eth_netif_hostname(hostname))) {
      LOG_ERROR("set_eth_netif_hostname");
      return err;
    }

    return 0;
  }

  static int config_eth_ip(const struct eth_config *config)
  {
    int err;

    LOG_INFO("Using config IPv4: ip=%s netmask=%s gw=%s", config->ip, config->netmask, config->gw);

    if ((err = set_eth_netif_ip(config->ip, config->netmask, config->gw))) {
      LOG_ERROR("set_eth_netif_ip");
      return err;
    }

    return 0;
  }

  static int config_eth_none(const struct eth_config *config)
  {
    int err;

    LOG_INFO("Using no IPv4 addressing mode");

    if ((err = config_eth_hostname(config))) {
      LOG_ERROR("config_eth_hostname");
      return err;
    }

    if ((err = set_eth_netif_static())) {
      LOG_ERROR("set_eth_netif_static");
      return err;
    }

    return 0;
  }

  static int config_eth_static(const struct eth_config *config)
  {
    int err;

    LOG_INFO("Using IPv4 Static");

    if ((err = config_eth_hostname(config))) {
      LOG_ERROR("config_eth_hostname");
      return err;
    }

    if ((err = set_eth_netif_static())) {
      LOG_ERROR("set_eth_netif_static");
      return err;
    }

    if ((err = config_eth_ip(config))) {
      LOG_ERROR("config_eth_ip");
      return err;
    }

    return 0;
  }

  static int config_eth_dhcpc(const struct eth_config *config)
  {
    int err;

    LOG_INFO("Using IPv4 DHCP Client");

    if ((err = config_eth_hostname(config))) {
      LOG_ERROR("config_eth_hostname");
      return err;
    }

    if ((err = set_eth_netif_dhcpc())) {
      LOG_ERROR("set_eth_netif_dhcpc");
      return err;
    }

    return 0;
  }

  static int config_eth_dhcps(const struct eth_config *config)
  {
    int err;

    LOG_INFO("Using IPv4 DHCP Server");

    if ((err = config_eth_hostname(config))) {
      LOG_ERROR("config_eth_hostname");
      return err;
    }

    // must be ESP_NETIF_DHCP_STOPPED to set static IP, ESP_NETIF_DHCP_INIT is not enough
    if ((err = set_eth_netif_static())) {
      LOG_ERROR("set_eth_netif_static");
      return err;
    }

    if ((err = config_eth_ip(config))) {
      LOG_ERROR("config_eth_ip");
      return err;
    }

    if ((err = set_eth_netif_dhcps())) {
      LOG_ERROR("set_eth_netif_dhcps");
      return err;
    }

    return 0;
  }

  int config_eth()
  {
    const struct eth_config *config = &eth_config;

    switch (config->mode) {
      case ETH_MODE_NONE:
        return config_eth_none(config);

      case ETH_MODE_STATIC:
        return config_eth_static(config);

      case ETH_MODE_DHCP_CLIENT:
        return config_eth_dhcpc(config);

      case ETH_MODE_DHCP_SERVER:
        return config_eth_dhcps(config);

      default:
        LOG_ERROR("invalid mode=%d", config->mode);
        return -1;
    }
  }
#endif
