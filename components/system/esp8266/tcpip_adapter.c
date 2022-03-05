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

static unsigned ipv4_netmask_prefixlen(const struct ip4_addr *ip)
{
  u32_t addr = lwip_htonl(ip4_addr_get_u32(ip));
  unsigned len = 0;

  for (u32_t mask = 1 << 31; mask; mask >>= 1) {
    if (addr & mask) {
      len++;
    } else {
      break;
    }
  }

  return len;
}

int system_interface_info(struct system_interface_info *info, tcpip_adapter_if_t tcpip_if)
{
  tcpip_adapter_ip_info_t ipv4;
  tcpip_adapter_dns_info_t dns_main = {}, dns_backup = {}, dns_fallback = {}; // initialize ip_addr_t type
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

  if ((err = tcpip_adapter_get_ip_info(tcpip_if, &ipv4))) {
    LOG_ERROR("tcpip_adapter_get_ip_info: %s", esp_err_to_name(err));
    return -1;
  } else {
    info->ipv4_address = ipv4.ip;
    info->ipv4_netmask = ipv4.netmask;
    info->ipv4_gateway = ipv4.gw;

    ip4_addr_get_network(&info->ipv4_network, &ipv4.ip, &ipv4.netmask);

    info->ipv4_prefixlen = ipv4_netmask_prefixlen(&ipv4.netmask);
  }

  if ((err = tcpip_adapter_get_dns_info(tcpip_if, TCPIP_ADAPTER_DNS_MAIN, &dns_main))) {
    LOG_ERROR("tcpip_adapter_get_dns_info TCPIP_ADAPTER_DNS_MAIN: %s", esp_err_to_name(err));
    return -1;
  } else {
    info->dns_main = dns_main.ip;
  }

  if ((err = tcpip_adapter_get_dns_info(tcpip_if, TCPIP_ADAPTER_DNS_BACKUP, &dns_backup))) {
    LOG_ERROR("tcpip_adapter_get_dns_info TCPIP_ADAPTER_DNS_BACKUP: %s", esp_err_to_name(err));
    return -1;
  } else {
    info->dns_backup = dns_backup.ip;
  }

  if ((err = tcpip_adapter_get_dns_info(tcpip_if, TCPIP_ADAPTER_DNS_FALLBACK, &dns_fallback))) {
    LOG_ERROR("tcpip_adapter_get_dns_info TCPIP_ADAPTER_DNS_FALLBACK: %s", esp_err_to_name(err));
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

  for (tcpip_adapter_if_t interface = 0; interface < TCPIP_ADAPTER_IF_MAX; interface++) {
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
