#ifndef __USER_INFO_H__
#define __USER_INFO_H__

#include <lwip/ip_addr.h>

enum user_event {
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_DISCONNECTED,
};

struct user_info {
  uint8_t   mac[6];
  bool      connected;
  char      hostname[32];
  ip_addr_t ip;
};

typedef void (*user_func_t)(const struct user_info *info, enum user_event event);

#endif
