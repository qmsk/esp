#pragma once

#include <system_interfaces.h>

/*
 * Check if system connected
 */
bool get_system_connected();

/* Get connected interface MAC address.
 *
 * @return <0 on error, 1 if not available
 */
int get_system_mac(uint8_t mac[6]);

/*
 * Get connected interface hostname.
 *
 * @return <0 on error, 1 if not available
 */
int get_system_hostname(const char **hostnamep);


/* Get connected interface IP address.
 *
 * @return <0 on error, 1 if not available
 */
int get_system_ipv4_addr(SYSTEM_IPV4_ADDR_TYPE *ip_addr);
