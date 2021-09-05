#include "dmx.h"
#include "dmx_config.h"

#include <artnet.h>
#include <gpio_out.h>

struct dmx_config dmx_configs[DMX_COUNT] = {
  [0] = {
   .gpio_mode         = -1,
  },
  [1] = {
    .gpio_mode        = -1,
  },
};

const struct config_enum dmx_gpio_mode_enum[] = {
 { "OFF",  -1              },
 { "HIGH", GPIO_OUT_HIGH   },
 { "LOW",  GPIO_OUT_LOW    },
 {}
};

#define DMX_CONFIGTAB dmx_configtab0
#define DMX_CONFIG dmx_configs[0]
#include "dmx_configtab.i"
#undef DMX_CONFIGTAB
#undef DMX_CONFIG

#define DMX_CONFIGTAB dmx_configtab1
#define DMX_CONFIG dmx_configs[1]
#include "dmx_configtab.i"
#undef DMX_CONFIGTAB
#undef DMX_CONFIG
