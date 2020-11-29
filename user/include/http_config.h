#ifndef USER_HTTP_CONFIG_H
#define USER_HTTP_CONFIG_H

#include <lib/config.h>

#define HTTP_CONFIG_HOST "0.0.0.0"
#define HTTP_CONFIG_PORT 80

struct http_config {
  char     host[32];
  uint16_t port;
};

extern struct http_config http_config;

extern const struct configtab http_configtab[];

#endif
