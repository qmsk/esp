#include <artnet.h>
#include "artnet.h"
#include "artnet_state.h"
#include "artnet_config.h"

#include <sdkconfig.h>


#if CONFIG_ARTNET_ENABLED
  #define ARTNET_CONFIG_ENABLED_DEFAULT true
#else
  #define ARTNET_CONFIG_ENABLED_DEFAULT false
#endif

struct artnet_config artnet_config = {

};

const struct configtab artnet_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &artnet_config.enabled, .default_value = ARTNET_CONFIG_ENABLED_DEFAULT },
  },
  { CONFIG_TYPE_UINT16, "net",
    .description = "Base network address: 0-127",
    .uint16_type = { .value = &artnet_config.net, .max = ARTNET_NET_MAX },
  },
  { CONFIG_TYPE_UINT16, "subnet",
    .description = "Base sub-net address: 0-16",
    .uint16_type = { .value = &artnet_config.subnet, .max = ARTNET_SUBNET_MAX },
  },
  {}
};
