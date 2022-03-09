#include "../wifi.h"
#include "../wifi_interface.h"
#include "../wifi_config.h"
#include "system_tcpip_adapter.h"

#include <logging.h>
#include <system_interfaces.h>
#include <system_wifi.h>

#include <esp_err.h>
#include <esp_wifi.h>
#include <mdns.h>
#include <tcpip_adapter.h>

int init_wifi_interface(wifi_interface_t interface)
{
  // no-op, handled by esp_wifi
  return 0;
}

tcpip_adapter_if_t tcpip_if_for_wifi_interface(wifi_interface_t interface)
{
  switch (interface) {
    case WIFI_IF_STA:
      return TCPIP_ADAPTER_IF_STA;

    case WIFI_IF_AP:
      return TCPIP_ADAPTER_IF_AP;

    default:
      return TCPIP_ADAPTER_IF_MAX;
  }
}

int set_wifi_interface_hostname(wifi_interface_t interface, const char *hostname)
{
  tcpip_adapter_if_t tcpip_if;
  esp_err_t err;

  if ((tcpip_if = tcpip_if_for_wifi_interface(interface)) >= TCPIP_ADAPTER_IF_MAX) {
    LOG_ERROR("invalid interface=%d", interface);
    return -1;
  }

  // TODO: multiple interfaces?
  if ((err = mdns_hostname_set(hostname))) {
    LOG_ERROR("mdns_hostname_set");
    return -1;
  }

  return 0;
}

int set_wifi_interface_ip(wifi_interface_t interface, const char *ip, const char *netmask, const char *gw)
{
  tcpip_adapter_if_t tcpip_if;
  tcpip_adapter_ip_info_t ip_info;
  esp_err_t err;

  if ((tcpip_if = tcpip_if_for_wifi_interface(interface)) >= TCPIP_ADAPTER_IF_MAX) {
    LOG_ERROR("invalid interface=%d", interface);
    return -1;
  }

  if (!ip4addr_aton(ip, &ip_info.ip)) {
    LOG_ERROR("invalid ip=%s", ip);
    return -1;
  }
  if (!ip4addr_aton(netmask, &ip_info.netmask)) {
    LOG_ERROR("invalid netmask=%s", netmask);
    return -1;
  }
  if (!ip4addr_aton(gw, &ip_info.gw)) {
    LOG_ERROR("invalid gw=%s", gw);
    return -1;
  }

  if (!(err = tcpip_adapter_dhcpc_stop(tcpip_if))) {

  } else if (err == ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED) {
    LOG_WARN("tcpip_adapter_dhcpc_stop: ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED");
  } else {
    LOG_ERROR("tcpip_adapter_dhcpc_stop: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_set_ip_info(tcpip_if, &ip_info))) {
    LOG_ERROR("tcpip_adapter_set_ip_info: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

void on_wifi_interface_up(wifi_interface_t interface, bool connected)
{
  char hostname[TCPIP_HOSTNAME_MAX_SIZE];
  tcpip_adapter_if_t tcpip_if;
  esp_err_t err;

  if ((tcpip_if = tcpip_if_for_wifi_interface(interface)) >= TCPIP_ADAPTER_IF_MAX) {
    LOG_ERROR("invalid interface=%d", interface);
    return;
  }

  // can only set hostname once tcpip_adapter is ready
  get_wifi_hostname(interface, &wifi_config, hostname, sizeof(hostname));

  LOG_INFO("set wifi_interface=%s -> tcpip_adapter=%s hostname=%s",
    wifi_interface_str(interface),
    tcpip_adapter_if_str(tcpip_if),
    hostname
  );

  if ((err = tcpip_adapter_set_hostname(tcpip_if, hostname))) {
    LOG_ERROR("tcpip_adapter_set_hostname %s: %s", tcpip_adapter_if_str(tcpip_if), esp_err_to_name(err));
  }

  system_tcpip_adapter_up(tcpip_if);
}

void on_wifi_interface_down(wifi_interface_t interface)
{
  tcpip_adapter_if_t tcpip_if;

  if ((tcpip_if = tcpip_if_for_wifi_interface(interface)) >= TCPIP_ADAPTER_IF_MAX) {
    LOG_ERROR("invalid interface=%d", interface);
    return;
  }

  system_tcpip_adapter_down(tcpip_if);
}
