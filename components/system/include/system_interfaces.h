#ifndef __SYSTEM_INTERFACES_H__
#define __SYSTEM_INTERFACES_H__

#include <tcpip_adapter.h>

#include <stdbool.h>

const char *tcpip_adapter_if_str(tcpip_adapter_if_t tcpip_if);
const char *tcpip_adapter_dhcp_status_str(tcpip_adapter_dhcp_status_t status);

struct system_interface_info {
  const char *hostname;
  tcpip_adapter_dhcp_status_t dhcps_status, dhcpc_status;
  tcpip_adapter_ip_info_t ipv4;
  tcpip_adapter_dns_info_t dns_main, dns_backup, dns_fallback;
};

/*
 * Returns <0 on error, 0 if valid, 1 if not configured/enabled/available.
 */
int system_interface_info(struct system_interface_info *info, tcpip_adapter_if_t tcpip_if);

#endif
