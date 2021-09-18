#ifndef __JSON_IP_H__
#define __JSON_IP_H__

#include <json.h>

#include "lwip/ip_addr.h"

int json_write_ipv4(struct json_writer *w, ip4_addr_t *ip);
int json_write_ipv6(struct json_writer *w, ip6_addr_t *ip);
int json_write_ip(struct json_writer *w, ip_addr_t *ip);

#define JSON_WRITE_MEMBER_IPV4(w, name, ip) (json_open_object_member((w), (name)) || json_write_ipv4((w), ip))
#define JSON_WRITE_MEMBER_IPV6(w, name, ip) (json_open_object_member((w), (name)) || json_write_ipv6((w), ip))
#define JSON_WRITE_MEMBER_IP(w, name, ip) (json_open_object_member((w), (name)) || json_write_ip((w), ip))

#endif
