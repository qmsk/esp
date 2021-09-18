#include <system_interfaces.h>

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
