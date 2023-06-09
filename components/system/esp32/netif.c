#include <system_interfaces.h>

#include <logging.h>

#include <esp_err.h>
#include <esp_netif.h>
#include <esp_netif_sta_list.h>
#include <esp_wifi.h>

#include <string.h>

const char *esp_netif_dhcp_status_str(esp_netif_dhcp_status_t status)
{
  switch (status) {
    case ESP_NETIF_DHCP_INIT:         return "INIT";
    case ESP_NETIF_DHCP_STARTED:      return "STARTED";
    case ESP_NETIF_DHCP_STOPPED:      return "STOPPED";
    default:                          return "?";
  }
}

static esp_ip4_addr_t ipv4_network(const esp_netif_ip_info_t *ip_info)
{
  uint32_t network = ip_info->ip.addr & ip_info->netmask.addr;

  return (esp_ip4_addr_t){ network };
}

static unsigned ipv4_prefixlen(const esp_netif_ip_info_t *ip_info)
{
  uint32_t addr = esp_netif_htonl(ip_info->netmask.addr);
  unsigned len = 0;

  for (uint32_t mask = 1 << 31; mask; mask >>= 1) {
    if (addr & mask) {
      len++;
    } else {
      break;
    }
  }

  return len;
}

int system_interface_info(struct system_interface_info *info, esp_netif_t *interface)
{
  esp_netif_ip_info_t ipv4;
  esp_netif_dns_info_t dns_main = {}, dns_backup = {}, dns_fallback = {}; // initialize ip_addr_t type
  esp_err_t err;

  info->interface = interface;

  if (!esp_netif_is_netif_up(interface)) {
    return 1;
  }

  if ((err = esp_netif_get_hostname(interface, &info->hostname))) {
    LOG_ERROR("esp_netif_get_hostname: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_netif_dhcps_get_status(interface, &info->dhcps_status))) {
    if (err == ESP_ERR_INVALID_ARG) {
      // not configured
      info->dhcps_status = ESP_NETIF_DHCP_INIT;
    } else {
      LOG_ERROR("esp_netif_dhcps_get_status: %s", esp_err_to_name(err));
      return -1;
    }
  }

  if ((err = esp_netif_dhcpc_get_status(interface, &info->dhcpc_status))) {
    if (err == ESP_ERR_INVALID_ARG) {
      // not configured
      info->dhcpc_status = ESP_NETIF_DHCP_INIT;
    } else {
      LOG_ERROR("esp_netif_dhcpc_get_status: %s", esp_err_to_name(err));
      return -1;
    }
  }

  if ((err = esp_netif_get_ip_info(interface, &ipv4))) {
    LOG_ERROR("esp_netif_get_ip_info: %s", esp_err_to_name(err));
    return -1;
  } else {
    info->ipv4_address = ipv4.ip;
    info->ipv4_netmask = ipv4.netmask;
    info->ipv4_gateway = ipv4.gw;
    info->ipv4_network = ipv4_network(&ipv4);
    info->ipv4_prefixlen = ipv4_prefixlen(&ipv4);
  }

  if ((err = esp_netif_get_dns_info(interface, ESP_NETIF_DNS_MAIN, &dns_main))) {
    LOG_ERROR("esp_netif_get_dns_info ESP_NETIF_DNS_MAIN: %s", esp_err_to_name(err));
    return -1;
  } else {
    info->dns_main = dns_main.ip;
  }

  if ((err = esp_netif_get_dns_info(interface, ESP_NETIF_DNS_BACKUP, &dns_backup))) {
    LOG_ERROR("esp_netif_get_dns_info ESP_NETIF_DNS_BACKUP: %s", esp_err_to_name(err));
    return -1;
  } else {
    info->dns_backup = dns_backup.ip;
  }

  if ((err = esp_netif_get_dns_info(interface, ESP_NETIF_DNS_FALLBACK, &dns_fallback))) {
    LOG_ERROR("esp_netif_get_dns_info ESP_NETIF_DNS_FALLBACK: %s", esp_err_to_name(err));
    return -1;
  } else {
    info->dns_fallback = dns_fallback.ip;
  }

  return 0;
}

int system_interface_walk(int (*func)(const struct system_interface_info *info, void *ctx), void *ctx)
{
  struct system_interface_info info;
  int err;

  for (esp_netif_t *interface = esp_netif_next(NULL); interface; interface = esp_netif_next(interface)) {
    if ((err = system_interface_info(&info, interface)) < 0) {
      LOG_ERROR("system_interface_info");
      return err;
    } else if (err) {
      continue;
    }

    if ((err = func(&info, ctx))) {
      return err;
    }
  }

  return 0;
}

int system_interface_clients_walk(int (*func)(const struct system_interface_client *client, void *ctx), void *ctx)
{
  struct system_interface_client client;
  wifi_sta_list_t wifi_sta_list;
  esp_netif_sta_list_t netif_sta_list;
  esp_err_t err;

  if ((err = esp_wifi_ap_get_sta_list(&wifi_sta_list))) {
    if (err == ESP_ERR_WIFI_MODE) {
      LOG_WARN("esp_wifi_ap_get_sta_list: %s", esp_err_to_name(err));
      return 0;
    } else {
      LOG_ERROR("esp_wifi_ap_get_sta_list: %s", esp_err_to_name(err));
      return -1;
    }
  }

  if ((err = esp_netif_get_sta_list(&wifi_sta_list, &netif_sta_list))) {
    LOG_ERROR("esp_netif_get_sta_list: %s", esp_err_to_name(err));
    return -1;
  }

  for (int i = 0; i < wifi_sta_list.num && i < netif_sta_list.num; i++) {
    esp_netif_sta_info_t *netif_sta_info = &netif_sta_list.sta[i];

    memcpy(&client.mac, &netif_sta_info->mac, sizeof(netif_sta_info->mac));
    memcpy(&client.ipv4, &netif_sta_info->ip, sizeof(netif_sta_info->ip));

    if ((err = func(&client, ctx))) {
      return err;
    }
  }

  return 0;
}
