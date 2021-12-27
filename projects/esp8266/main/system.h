#pragma once

#include <cmd.h>
#include <lwip/ip_addr.h>

extern const struct cmdtab system_cmdtab;

/*
 * Initialize system components
 */
int init_system();


/* get WiFI AP/STA MAC address.
 *
 * @return <0 on error, 1 if not available
 */
int get_system_mac(uint8_t mac[6]);

/*
 * Get WiFi AP/STA hostname.
 *
 * @return <0 on error, 1 if not available
 */
int get_system_hostname(const char **hostnamep);


/* get WiFI AP/STA IP address.
 *
 * @return <0 on error, 1 if not available
 */
int get_system_ipv4_addr(ip4_addr_t *ip_addr);

/*
 * Soft system reset.
 */
void system_restart() __attribute__ ((noreturn));
