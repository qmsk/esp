#include <artnet.h>
#include "artnet.h"
#include "artnet_state.h"
#include "artnet_config.h"
#include "dmx_config.h"
#include "leds_config.h"

#include <sdkconfig.h>
#include <logging.h>

#if CONFIG_ARTNET_ENABLED
  #define ARTNET_CONFIG_ENABLED_DEFAULT true
#else
  #define ARTNET_CONFIG_ENABLED_DEFAULT false
#endif

struct artnet_config artnet_config = {

};

static int artnet_migrate_net(const struct config_path path, uint16_t value)
{
  LOG_INFO("dmx-input.artnet_net = %u", value);
  dmx_input_config.artnet_net = value;

  for (unsigned i = 0; i < DMX_OUTPUT_COUNT; i++) {
    LOG_INFO("dmx-output%d.artnet_net = %u", i+1, value);
    dmx_output_configs[i].artnet_net = value;
  }

  for (unsigned i = 0; i < LEDS_COUNT; i++) {
    LOG_INFO("leds%d.artnet_net = %u", i+1, value);
    leds_configs[i].artnet_net = value;
  }

  return 0;
}

static int artnet_migrate_subnet(const struct config_path path, uint16_t value)
{
  LOG_INFO("dmx-input.artnet_subnet = %u", value);
  dmx_input_config.artnet_subnet = value;

  for (unsigned i = 0; i < DMX_OUTPUT_COUNT; i++) {
    LOG_INFO("dmx-output%d.artnet_subnet = %u", i+1, value);
    dmx_output_configs[i].artnet_subnet = value;
  }

  for (unsigned i = 0; i < LEDS_COUNT; i++) {
    LOG_INFO("leds%d.artnet_subnet = %u", i+1, value);
    leds_configs[i].artnet_subnet = value;
  }

  return 0;
}

const struct configtab artnet_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &artnet_config.enabled, .default_value = ARTNET_CONFIG_ENABLED_DEFAULT },
  },
  { CONFIG_TYPE_UINT16, "net",
    .description = "Base network address: 0-127",
    .migrated = true,
    .uint16_type = { .max = ARTNET_NET_MAX, .migrate_func = artnet_migrate_net },
  },
  { CONFIG_TYPE_UINT16, "subnet",
    .description = "Base sub-net address: 0-16",
    .migrated = true,
    .uint16_type = { .max = ARTNET_SUBNET_MAX, .migrate_func = artnet_migrate_subnet },
  },
  {}
};
