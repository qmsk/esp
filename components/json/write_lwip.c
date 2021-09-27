#include <json.h>
#include <json_lwip.h>
#include "write.h"

#include <logging.h>

#define JSON_QUOTE "\""

int json_write_ipv4(struct json_writer *w, ip4_addr_t *ip)
{
  return json_writef(w, JSON_STRING, (JSON_QUOTE "%u.%u.%u.%u" JSON_QUOTE),
    ip4_addr1(ip),
    ip4_addr2(ip),
    ip4_addr3(ip),
    ip4_addr4(ip)
  );
}

int json_write_ipv6(struct json_writer *w, ip6_addr_t *ip)
{
  return json_writef(w, JSON_STRING, (JSON_QUOTE "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x" JSON_QUOTE),
    IP6_ADDR_BLOCK1(ip),
    IP6_ADDR_BLOCK2(ip),
    IP6_ADDR_BLOCK3(ip),
    IP6_ADDR_BLOCK4(ip),
    IP6_ADDR_BLOCK5(ip),
    IP6_ADDR_BLOCK6(ip),
    IP6_ADDR_BLOCK7(ip),
    IP6_ADDR_BLOCK8(ip)
 );
}

int json_write_ip(struct json_writer *w, ip_addr_t *ip)
{
  switch (IP_GET_TYPE(ip)) {
    case IPADDR_TYPE_V4:
      return json_write_ipv4(w, ip_2_ip4(ip));

    case IPADDR_TYPE_V6:
      return json_write_ipv6(w, ip_2_ip6(ip));

    default:
      LOG_WARN("unknown IP type=%d", IP_GET_TYPE(ip));
      return json_write_null(w);
  }
}
