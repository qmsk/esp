#pragma once

#include <tcpip_adapter.h>

void system_tcpip_adapter_up(tcpip_adapter_if_t tcpip_if);
void system_tcpip_adapter_connected(tcpip_adapter_if_t tcpip_if);
void system_tcpip_adapter_disconnected();
void system_tcpip_adapter_down(tcpip_adapter_if_t tcpip_if);
