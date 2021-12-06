#include "dmx.h"
#include "dmx_config.h"

#include <artnet.h>
#include <gpio_out.h>

struct dmx_input_config dmx_input_config = {

};

struct dmx_output_config dmx_output_configs[DMX_OUTPUT_COUNT] = {
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

const struct configtab dmx_input_configtab[] = {
 { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &dmx_input_config.enabled },
 },
 { CONFIG_TYPE_BOOL, "artnet_enabled",
    .description = "Configure Art-NET input port.",
    .bool_type = { .value = &dmx_input_config.artnet_enabled },
 },
 { CONFIG_TYPE_UINT16, "artnet_universe",
    .description = "Input to universe (0-15) within [artnet] net/subnet.",
    .uint16_type = { .value = &dmx_input_config.artnet_universe, .max = ARTNET_UNIVERSE_MAX },
 },
 {}
};

#define DMX_OUTPUT_CONFIGTAB dmx_output_configtab0
#define DMX_OUTPUT_CONFIG dmx_output_configs[0]
#include "dmx_output_configtab.i"
#undef DMX_OUTPUT_CONFIGTAB
#undef DMX_OUTPUT_CONFIG

#define DMX_OUTPUT_CONFIGTAB dmx_output_configtab1
#define DMX_OUTPUT_CONFIG dmx_output_configs[1]
#include "dmx_output_configtab.i"
#undef DMX_OUTPUT_CONFIGTAB
#undef DMX_OUTPUT_CONFIG
