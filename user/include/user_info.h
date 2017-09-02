#ifndef __USER_INFO_H__
#define __USER_INFO_H__

#include <lwip/ip_addr.h>

struct user_info {
  uint8_t   mac[6];
  bool      connected;
  char      hostname[32];
  ip_addr_t ip;
};

#endif
