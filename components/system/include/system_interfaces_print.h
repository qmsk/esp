#pragma once

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP8266
# include <tcpip_adapter.h>
#elif CONFIG_IDF_TARGET_ESP32
# include <esp_netif.h>
#endif


#if CONFIG_IDF_TARGET_ESP8266

  static inline void print_ip4_address(const ip4_addr_t *ip)
  {
    printf("%u.%u.%u.%u",
      ip4_addr1(ip),
      ip4_addr2(ip),
      ip4_addr3(ip),
      ip4_addr4(ip)
    );
  }

  static inline void print_ip4_cidr(const ip4_addr_t *ip, unsigned prefixlen)
  {
    printf("%u.%u.%u.%u/%u",
      ip4_addr1(ip),
      ip4_addr2(ip),
      ip4_addr3(ip),
      ip4_addr4(ip),
      prefixlen
    );
  }

  static inline void print_ip6_address(const ip6_addr_t *ip)
  {
    printf("%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
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

  static inline void print_ip_address(const ip_addr_t *ip)
  {
    if (IP_IS_V4(ip)) {
      print_ip4_address(ip_2_ip4(ip));
    } else if (IP_IS_V6(ip)) {
      print_ip6_address(ip_2_ip6(ip));
    } else {
      printf("??? (type=%d)", IP_GET_TYPE(ip));
    }
  }

#elif CONFIG_IDF_TARGET_ESP32

  static inline void print_ip4_address(const esp_ip4_addr_t *ip)
  {
    printf("%u.%u.%u.%u",
      esp_ip4_addr1(ip),
      esp_ip4_addr2(ip),
      esp_ip4_addr3(ip),
      esp_ip4_addr4(ip)
    );
  }

  static inline void print_ip4_cidr(const esp_ip4_addr_t *ip, unsigned prefixlen)
  {
    printf("%u.%u.%u.%u/%u",
      esp_ip4_addr1(ip),
      esp_ip4_addr2(ip),
      esp_ip4_addr3(ip),
      esp_ip4_addr4(ip),
      prefixlen
    );
  }

  static inline void print_ip6_address(const esp_ip6_addr_t *ip)
  {
    printf("%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
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

  static inline void print_ip_address(const esp_ip_addr_t *ip)
  {
    switch (ip->type) {
      case ESP_IPADDR_TYPE_V4:
       return print_ip4_address(&ip->u_addr.ip4);

      case ESP_IPADDR_TYPE_V6:
       return print_ip6_address(&ip->u_addr.ip6);

     default:
       printf("??? (type=%d)", IP_GET_TYPE(ip));
    }
  }
#endif
