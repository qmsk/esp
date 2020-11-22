#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__

#include <lib/config.h>

struct wifi_config {
  char ssid[32];
  char password[64];
};

extern struct wifi_config wifi_config;

extern const struct configtab wifi_configtab[];

#endif
