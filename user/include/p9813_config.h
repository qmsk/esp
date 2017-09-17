#ifndef __P9813_CONFIG_H__
#define __P9813_CONFIG_H__

struct p9813_config {
  uint16_t count;
  uint16_t artnet_universe;
  uint16_t gpio; // output power, active high
};

#endif
