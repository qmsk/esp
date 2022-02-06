#include <json.h>
#include <json_netif.h>
#include "../write.h"

#include <logging.h>

#define JSON_QUOTE "\""

int json_write_ipv4(struct json_writer *w, const esp_ip4_addr_t *ip)
{
  return json_writef(w, JSON_STRING, (JSON_QUOTE "%u.%u.%u.%u" JSON_QUOTE),
    esp_ip4_addr1(ip),
    esp_ip4_addr2(ip),
    esp_ip4_addr3(ip),
    esp_ip4_addr4(ip)
  );
}

int json_write_ipv6(struct json_writer *w, const esp_ip6_addr_t *ip)
{
  return json_writef(w, JSON_STRING, (JSON_QUOTE "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x" JSON_QUOTE),
    ESP_IP6_ADDR_BLOCK1(ip),
    ESP_IP6_ADDR_BLOCK2(ip),
    ESP_IP6_ADDR_BLOCK3(ip),
    ESP_IP6_ADDR_BLOCK4(ip),
    ESP_IP6_ADDR_BLOCK5(ip),
    ESP_IP6_ADDR_BLOCK6(ip),
    ESP_IP6_ADDR_BLOCK7(ip),
    ESP_IP6_ADDR_BLOCK8(ip)
 );
}

int json_write_ip(struct json_writer *w, const esp_ip_addr_t *ip)
{
  switch (ip->type) {
    case ESP_IPADDR_TYPE_V4:
      return json_write_ipv4(w, &ip->u_addr.ip4);

    case ESP_IPADDR_TYPE_V6:
      return json_write_ipv6(w, &ip->u_addr.ip6);

    default:
      LOG_WARN("unknown IP type=%d", ip->type);
      return json_write_null(w);
  }
}
