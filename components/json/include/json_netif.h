#pragma once

#include <json.h>

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP32
# include <esp_netif_ip_addr.h>

  int json_write_ipv4(struct json_writer *w, const esp_ip4_addr_t *ip);
  int json_write_ipv6(struct json_writer *w, const esp_ip6_addr_t *ip);
  int json_write_ip(struct json_writer *w, const esp_ip_addr_t *ip);

  #define JSON_WRITE_MEMBER_IPV4(w, name, ip) (json_open_object_member((w), (name)) || json_write_ipv4((w), ip))
  #define JSON_WRITE_MEMBER_IPV6(w, name, ip) (json_open_object_member((w), (name)) || json_write_ipv6((w), ip))
  #define JSON_WRITE_MEMBER_IP(w, name, ip) (json_open_object_member((w), (name)) || json_write_ip((w), ip))
#else
# error "Not supported for" IDF_TARGET
#endif
