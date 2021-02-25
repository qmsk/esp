#ifndef _USER_APA102_INIT_H
#define _USER_APA102_INIT_H

#include <lib/config.h>
#include <lib/cmd.h>

struct apa102_config {
  bool enabled;
  bool stop_quirk;
  uint16_t count;
  uint16_t led_gpio;
  bool artnet_enabled;
  uint16_t artnet_universe;
};

extern struct apa102_config apa102_config;

extern const struct configtab apa102_configtab[];

int init_apa102(const struct apa102_config *config);

extern const struct cmdtab apa102_cmdtab;

#endif
