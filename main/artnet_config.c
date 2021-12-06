#include <artnet.h>
#include "artnet.h"
#include "artnet_config.h"

struct artnet_config artnet_config = {

};

const struct configtab artnet_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &artnet_config.enabled },
  },
  { CONFIG_TYPE_BOOL, "inputs_enabled",
    .description = (
      "Support Art-NET input ports."
    ),
    .bool_type = { .value = &artnet_config.inputs_enabled },
  },
  { CONFIG_TYPE_UINT16, "net",
    .description = "Set network address, 0-127.",
    .uint16_type = { .value = &artnet_config.net, .max = ARTNET_NET_MAX },
  },
  { CONFIG_TYPE_UINT16, "subnet",
    .description = "Set sub-net address, 0-16.",
    .uint16_type = { .value = &artnet_config.subnet, .max = ARTNET_SUBNET_MAX },
  },
  {}
};
