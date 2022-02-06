#pragma once

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP8266
# include <tcpip_adapter.h>
#elif CONFIG_IDF_TARGET_ESP32
# include <esp_netif.h>
#endif

#include <stdbool.h>

#if CONFIG_IDF_TARGET_ESP8266

  const char *tcpip_adapter_if_str(tcpip_adapter_if_t tcpip_if);
  const char *tcpip_adapter_dhcp_status_str(tcpip_adapter_dhcp_status_t status);

  struct system_interface_info {
    tcpip_adapter_if_t interface;

    const char *hostname;

    tcpip_adapter_dhcp_status_t dhcps_status, dhcpc_status;

    ip4_addr_t ipv4_address, ipv4_netmask, ipv4_gateway;
    ip4_addr_t ipv4_network;
    unsigned ipv4_prefixlen;

    ip_addr_t dns_main, dns_backup, dns_fallback;
  };


#elif CONFIG_IDF_TARGET_ESP32

  const char *esp_netif_dhcp_status_str(esp_netif_dhcp_status_t status);

  struct system_interface_info {
    esp_netif_t *interface;

    const char *hostname;

    esp_netif_dhcp_status_t dhcps_status, dhcpc_status;

    esp_ip4_addr_t ipv4_address, ipv4_netmask, ipv4_gateway;
    esp_ip4_addr_t ipv4_network;
    unsigned ipv4_prefixlen;

    esp_ip_addr_t dns_main, dns_backup, dns_fallback;
  };

  struct system_interface_client {
    uint8_t mac[6];
    esp_ip4_addr_t ipv4;
  };
#endif

#if CONFIG_IDF_TARGET_ESP8266
  static inline const char *system_interface_str(const struct system_interface_info *info)
  {
    return tcpip_adapter_if_str(info->interface);
  }

  static inline const char *system_interface_dhcp_status_str(tcpip_adapter_dhcp_status_t status)
  {
    return tcpip_adapter_dhcp_status_str(status);
  }
#elif CONFIG_IDF_TARGET_ESP32
  static inline const char *system_interface_str(const struct system_interface_info *info)
  {
    return esp_netif_get_desc(info->interface);
  }

  static inline const char *system_interface_dhcp_status_str(esp_netif_dhcp_status_t status)
  {
    return esp_netif_dhcp_status_str(status);
  }
#endif


/*
 * Returns <0 on error, 0 if valid, 1 if not configured/enabled/available.
 */
#if CONFIG_IDF_TARGET_ESP8266
  int system_interface_info(struct system_interface_info *info, tcpip_adapter_if_t interface);
#elif CONFIG_IDF_TARGET_ESP32
  int system_interface_info(struct system_interface_info *info, esp_netif_t *interface);

  int system_interface_walk(int (*func)(const struct system_interface_info *info, void *ctx), void *ctx);
  int system_interface_clients_walk(int (*func)(const struct system_interface_client *client, void *ctx), void *ctx);
#endif
