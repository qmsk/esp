#include "../wifi.h"
#include "../wifi_interface.h"
#include "system_netif.h"

#include <esp_err.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <mdns.h>

#include <logging.h>
#include <system_wifi.h>

esp_netif_t *wifi_netif[] = {
  [WIFI_IF_STA] = NULL,
  [WIFI_IF_AP]  = NULL,
};

static int create_wifi_netif(esp_netif_t **netifp, wifi_interface_t interface)
{
  switch(interface) {
    case WIFI_IF_STA:
      if (!(*netifp = esp_netif_create_default_wifi_sta())) {
        LOG_ERROR("esp_netif_create_default_wifi_sta");
        return -1;
      }

      return 0;

    case WIFI_IF_AP:
      if (!(*netifp = esp_netif_create_default_wifi_ap())) {
        LOG_ERROR("esp_netif_create_default_wifi_ap");
        return -1;
      }

      return 0;

    default:
      LOG_ERROR("invalid interface=%d", interface);
      return -1;
  }
}

int make_wifi_netif(esp_netif_t **netifp, wifi_interface_t interface)
{
  esp_netif_t *netif;
  int err;

  if ((netif = wifi_netif[interface])) {

  } else if ((err = create_wifi_netif(&netif, interface))) {
    LOG_ERROR("create_wifi_netif(%d)", interface);
    return err;
  } else {
    LOG_INFO("created wifi_netif[%s]: %s", wifi_interface_str(interface), esp_netif_get_desc(netif));

    wifi_netif[interface] = netif;
  }

  if (netifp) {
    *netifp = netif;
  }

  return 0;
}

int init_wifi_interface(wifi_interface_t interface)
{
  return make_wifi_netif(NULL, interface);
}

int set_wifi_interface_hostname(wifi_interface_t interface, const char *hostname)
{
  esp_netif_t *netif;
  esp_err_t err;

  if ((err = make_wifi_netif(&netif, interface))) {
    LOG_ERROR("make_wifi_netif");
    return err;
  } else if ((err = esp_netif_set_hostname(netif, hostname))) {
    LOG_ERROR("esp_netif_set_hostname: %s", esp_err_to_name(err));
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
  esp_netif_t *netif;
  esp_netif_ip_info_t ip_info;
  esp_err_t err;

  if ((err = esp_netif_str_to_ip4(ip, &ip_info.ip))) {
    LOG_ERROR("invalid ip=%s", ip);
    return -1;
  }

  if ((err = esp_netif_str_to_ip4(netmask, &ip_info.netmask))) {
    LOG_ERROR("invalid netmask=%s", netmask);
    return -1;
  }

  if ((err = esp_netif_str_to_ip4(gw, &ip_info.gw))) {
    LOG_ERROR("invalid gw=%s", gw);
    return -1;
  }

  if ((err = make_wifi_netif(&netif, interface))) {
    LOG_ERROR("make_wifi_netif");
    return err;
  }

  if (!(err = esp_netif_dhcpc_stop(netif))) {

  } else if (err == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
    LOG_WARN("esp_netif_dhcpc_stop: ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED");
  } else {
    LOG_ERROR("esp_netif_dhcpc_stop: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_netif_set_ip_info(netif, &ip_info))) {
    LOG_ERROR("esp_netif_set_ip_info: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int set_wifi_interface_dhcps_options(wifi_interface_t interface)
{
  esp_netif_t *netif;
  esp_err_t err;

  if ((err = make_wifi_netif(&netif, interface))) {
    LOG_ERROR("make_wifi_netif");
    return err;
  }

  // disable OFFER_ROUTER
  uint8_t dhcps_offer_router = 0;

  if ((err = esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_ROUTER_SOLICITATION_ADDRESS, &dhcps_offer_router, sizeof(dhcps_offer_router)))) {
    LOG_ERROR("esp_netif_dhcps_option ESP_NETIF_OP_SET ESP_NETIF_ROUTER_SOLICITATION_ADDRESS: %s", esp_err_to_name(err));
    return -1;
  }

  // disable OFFER_DNS
  uint8_t dhcps_offer_dns = 0;

  if ((err = esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_dns, sizeof(dhcps_offer_dns)))) {
    LOG_ERROR("esp_netif_dhcps_option ESP_NETIF_OP_SET ESP_NETIF_DOMAIN_NAME_SERVER: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

void on_wifi_interface_up(wifi_interface_t interface, bool connected)
{
  esp_netif_t *netif;

  if (!(netif = wifi_netif[interface])) {
    LOG_ERROR("unknown netif for interface=%d", interface);
    return;
  }

  system_netif_up(netif);

  if (connected) {
    system_netif_connected(netif);
  }
}

void on_wifi_interface_down(wifi_interface_t interface)
{
  esp_netif_t *netif;

  if (!(netif = wifi_netif[interface])) {
    LOG_ERROR("unknown netif for interface=%d", interface);
    return;
  }

  system_netif_down(netif);
}
