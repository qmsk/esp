#ifndef __SYSTEM_INTERFACES_H__
#define __SYSTEM_INTERFACES_H__

#include <tcpip_adapter.h>

const char *tcpip_adapter_if_str(tcpip_adapter_if_t tcpip_if);
const char *tcpip_adapter_dhcp_status_str(tcpip_adapter_dhcp_status_t status);

#endif
