#ifndef __USER_P9813_H__
#define __USER_P9813_H__

#include <lib/cmd.h>
#include <lib/config.h>

struct p9813_config {
  bool enabled;
  uint16_t count;
  uint16_t artnet_universe;
  uint16_t gpio; // output power, active high
};

extern struct p9813_config p9813_config;

extern const struct configtab p9813_configtab[];

int init_p9813 (const struct p9813_config *config);

extern const struct cmdtab p9813_cmdtab;

#endif
