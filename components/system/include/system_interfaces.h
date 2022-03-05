#pragma once

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP8266

# include <tcpip_adapter.h>

# define SYSTEM_INTERFACE_TYPE tcpip_adapter_if_t
# define SYSTEM_DHCP_STATUS_TYPE tcpip_adapter_dhcp_status_t
# define SYSTEM_IPV4_ADDR_TYPE ip4_addr_t
# define SYSTEM_IPV4_ADDR_BYTES(x) ip4_addr1(x), ip4_addr2(x), ip4_addr3(x), ip4_addr4(x)
# define SYSTEM_IPV4_ADDR_BYTE1(x) ip4_addr1(x)
# define SYSTEM_IPV4_ADDR_BYTE2(x) ip4_addr2(x)
# define SYSTEM_IPV4_ADDR_BYTE3(x) ip4_addr3(x)
# define SYSTEM_IPV4_ADDR_BYTE4(x) ip4_addr4(x)
# define SYSTEM_IP_ADDR_TYPE ip_addr_t

  const char *tcpip_adapter_if_str(tcpip_adapter_if_t tcpip_if);
  const char *tcpip_adapter_dhcp_status_str(tcpip_adapter_dhcp_status_t status);

#elif CONFIG_IDF_TARGET_ESP32

# include <esp_netif.h>


# define SYSTEM_INTERFACE_TYPE esp_netif_t *
# define SYSTEM_DHCP_STATUS_TYPE esp_netif_dhcp_status_t
# define SYSTEM_IPV4_ADDR_TYPE esp_ip4_addr_t
# define SYSTEM_IPV4_ADDR_BYTES(x) esp_ip4_addr1(x), esp_ip4_addr2(x), esp_ip4_addr3(x), esp_ip4_addr4(x)
# define SYSTEM_IPV4_ADDR_BYTE1(x) esp_ip4_addr1(x)
# define SYSTEM_IPV4_ADDR_BYTE2(x) esp_ip4_addr2(x)
# define SYSTEM_IPV4_ADDR_BYTE3(x) esp_ip4_addr3(x)
# define SYSTEM_IPV4_ADDR_BYTE4(x) esp_ip4_addr4(x)
# define SYSTEM_IP_ADDR_TYPE esp_ip_addr_t

  const char *esp_netif_dhcp_status_str(esp_netif_dhcp_status_t status);

#endif

struct system_interface_info {
  SYSTEM_INTERFACE_TYPE interface;

  const char *hostname;

  SYSTEM_DHCP_STATUS_TYPE dhcps_status, dhcpc_status;

  SYSTEM_IPV4_ADDR_TYPE ipv4_address, ipv4_netmask, ipv4_gateway;
  SYSTEM_IPV4_ADDR_TYPE ipv4_network;
  unsigned ipv4_prefixlen;

  SYSTEM_IP_ADDR_TYPE dns_main, dns_backup, dns_fallback;
};

struct system_interface_client {
  uint8_t mac[6];
  SYSTEM_IPV4_ADDR_TYPE ipv4;
};

#if CONFIG_IDF_TARGET_ESP8266
  static inline const char *system_interface_str(const struct system_interface_info *info)
  {
    return tcpip_adapter_if_str(info->interface);
  }

  static inline const char *system_interface_dhcp_status_str(tcpip_adapter_dhcp_status_t status)
  {
    return tcpip_adapter_dhcp_status_str(status);
  }

  /*
   * Returns <0 on error, 0 if valid, 1 if not configured/enabled/available.
   */
  int system_interface_info(struct system_interface_info *info, tcpip_adapter_if_t interface);

#elif CONFIG_IDF_TARGET_ESP32

  static inline const char *system_interface_str(const struct system_interface_info *info)
  {
    return esp_netif_get_desc(info->interface);
  }

  static inline const char *system_interface_dhcp_status_str(esp_netif_dhcp_status_t status)
  {
    return esp_netif_dhcp_status_str(status);
  }

  /*
   * Returns <0 on error, 0 if valid, 1 if not configured/enabled/available.
   */
  int system_interface_info(struct system_interface_info *info, esp_netif_t *interface);


#endif

/*
 * Call func with info on each configured system network interface.
 */
int system_interface_walk(int (*func)(const struct system_interface_info *info, void *ctx), void *ctx);

/*
 * Call func with info on each connected wifi (client) station.
 */
int system_interface_clients_walk(int (*func)(const struct system_interface_client *client, void *ctx), void *ctx);
