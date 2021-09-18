#include <system_interfaces.h>

#include <logging.h>

#include <esp_err.h>

const char *tcpip_adapter_if_str(tcpip_adapter_if_t tcpip_if)
{
  switch(tcpip_if) {
    case TCPIP_ADAPTER_IF_STA:    return "STA";
    case TCPIP_ADAPTER_IF_AP:     return "AP";
    case TCPIP_ADAPTER_IF_ETH:    return "ETH";
    case TCPIP_ADAPTER_IF_TEST:   return "TEST";
    default:                      return "?";
  }
}

const char *tcpip_adapter_dhcp_status_str(tcpip_adapter_dhcp_status_t status)
{
  switch (status) {
    case TCPIP_ADAPTER_DHCP_INIT:         return "INIT";
    case TCPIP_ADAPTER_DHCP_STARTED:      return "STARTED";
    case TCPIP_ADAPTER_DHCP_STOPPED:      return "STOPPED";
    default:                              return "?";
  }
}

int system_interface_info(struct system_interface_info *info, tcpip_adapter_if_t tcpip_if)
{
  esp_err_t err;

  if (!tcpip_adapter_is_netif_up(tcpip_if)) {
    return 1;
  }

  if ((err = tcpip_adapter_get_hostname(tcpip_if, &info->hostname))) {
    LOG_ERROR("tcpip_adapter_get_hostname: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_dhcps_get_status(tcpip_if, &info->dhcps_status))) {
    LOG_ERROR("tcpip_adapter_dhcps_get_status: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_dhcpc_get_status(tcpip_if, &info->dhcpc_status))) {
    LOG_ERROR("tcpip_adapter_dhcpc_get_status: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_ip_info(tcpip_if, &info->ipv4))) {
    LOG_ERROR("tcpip_adapter_get_ip_info: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_dns_info(tcpip_if, TCPIP_ADAPTER_DNS_MAIN, &info->dns_main))) {
    LOG_ERROR("tcpip_adapter_get_dns_info TCPIP_ADAPTER_DNS_MAIN: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_dns_info(tcpip_if, TCPIP_ADAPTER_DNS_BACKUP, &info->dns_backup))) {
    LOG_ERROR("tcpip_adapter_get_dns_info TCPIP_ADAPTER_DNS_BACKUP: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_dns_info(tcpip_if, TCPIP_ADAPTER_DNS_FALLBACK, &info->dns_fallback))) {
    LOG_ERROR("tcpip_adapter_get_dns_info TCPIP_ADAPTER_DNS_FALLBACK: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
