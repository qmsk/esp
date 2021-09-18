#include <json.h>
#include <json_lwip.h>
#include "write.h"

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
  if (IP_IS_V4(ip)) {
    return json_write_ipv4(w, ip_2_ip4(ip));
  } else if (IP_IS_V6(ip)) {
    return json_write_ipv6(w, ip_2_ip6(ip));
  } else {
    return 1;
  }
}
