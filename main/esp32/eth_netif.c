#include "../eth.h"
#include "../eth_config.h"
#include "../eth_netif.h"
#include "../eth_state.h"

#include <esp_eth.h>
#include <esp_err.h>
#include <esp_netif.h>
#include <mdns.h>

#include <logging.h>

static esp_netif_t *eth_netif;
static esp_eth_netif_glue_handle_t eth_netif_glue;

int init_eth_netif()
{
  esp_eth_handle_t eth_handle = get_eth_handle();
  esp_netif_inherent_config_t netif_config_base = ESP_NETIF_INHERENT_DEFAULT_ETH();
  esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
  esp_err_t err;

  // The ESP_NETIF_DHCP_CLIENT/SERVER flags are mutually exclusive.
  // A ESP_NETIF_DHCP_CLIENT cannot start a DHCP server, and a ESP_NETIF_DHCP_SERVER cannot start a DHCP client.
  // With both flags set, esp_netif_dhcps/dhcpc_get_status() will both return ESP_ERR_INVALID_ARG.
  if (is_eth_dhcp_client()) {
    netif_config_base.flags |= ESP_NETIF_DHCP_CLIENT;
    netif_config_base.flags &= ~ESP_NETIF_DHCP_SERVER;
  } else if (is_eth_dhcp_server()) {
    netif_config_base.flags |= ESP_NETIF_DHCP_SERVER;
    netif_config_base.flags &= ~ESP_NETIF_DHCP_CLIENT;
  } else {
    netif_config_base.flags |= ESP_NETIF_DHCP_CLIENT; // required for static address -> ESP_NETIF_IP_EVENT_GOT_IP
    netif_config_base.flags &= ~ESP_NETIF_DHCP_SERVER;
  }

  netif_config.base = &netif_config_base;

  LOG_INFO("flags<DHCP_CLIENT=%d, DHCP_SERVER=%d>",
    netif_config_base.flags & ESP_NETIF_DHCP_CLIENT,
    netif_config_base.flags & ESP_NETIF_DHCP_SERVER
  );

  if (!(eth_netif = esp_netif_new(&netif_config))) {
    LOG_ERROR("esp_netif_new");
    return -1;
  }

  if (!(eth_netif_glue = esp_eth_new_netif_glue(eth_handle))) {
    LOG_ERROR("esp_eth_new_netif_glue");
    return -1;
  }

  if ((err = esp_netif_attach(eth_netif, eth_netif_glue))) {
    LOG_ERROR("esp_netif_attach: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int set_eth_netif_hostname(const char *hostname)
{
  esp_err_t err;

  if ((err = esp_netif_set_hostname(eth_netif, hostname))) {
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

int set_eth_netif_ip(const char *ip, const char *netmask, const char *gw)
{
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

  if ((err = esp_netif_set_ip_info(eth_netif, &ip_info))) {
    LOG_ERROR("esp_netif_set_ip_info: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int set_eth_netif_static()
{
  esp_err_t err;

  if ((err = esp_netif_dhcps_stop(eth_netif)) && (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)) {
    LOG_ERROR("esp_netif_dhcps_stop: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_netif_dhcpc_stop(eth_netif)) && (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)) {
    LOG_ERROR("esp_netif_dhcpc_stop: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int set_eth_netif_dhcpc()
{
  esp_err_t err;

  if ((err = esp_netif_dhcps_stop(eth_netif)) && (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)) {
    LOG_ERROR("esp_netif_dhcps_stop: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_netif_dhcpc_start(eth_netif)) && (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED)) {
    LOG_ERROR("esp_netif_dhcpc_start: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int set_eth_netif_dhcps()
{
  esp_err_t err;

  if ((err = esp_netif_dhcpc_stop(eth_netif)) && (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)) {
    LOG_ERROR("esp_netif_dhcpc_stop: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_netif_dhcps_start(eth_netif)) && (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)) {
    LOG_ERROR("esp_netif_dhcps_start: %s", esp_err_to_name(err));
    return -1;
  }

  // disable OFFER_ROUTER
  uint8_t dhcps_offer_router = 0;

  if ((err = esp_netif_dhcps_option(eth_netif, ESP_NETIF_OP_SET, ESP_NETIF_ROUTER_SOLICITATION_ADDRESS, &dhcps_offer_router, sizeof(dhcps_offer_router)))) {
    LOG_ERROR("esp_netif_dhcps_option ESP_NETIF_OP_SET ESP_NETIF_ROUTER_SOLICITATION_ADDRESS: %s", esp_err_to_name(err));
    return -1;
  }

  // disable OFFER_DNS
  uint8_t dhcps_offer_dns = 0;

  if ((err = esp_netif_dhcps_option(eth_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_dns, sizeof(dhcps_offer_dns)))) {
    LOG_ERROR("esp_netif_dhcps_option ESP_NETIF_OP_SET ESP_NETIF_DOMAIN_NAME_SERVER: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
