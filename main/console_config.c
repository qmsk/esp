#include "console.h"
#include "console_config.h"

struct console_config console_config = { };

const struct configtab console_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .description = "Start UART console at boot",
    .bool_type = { .value = &console_config.enabled, .default_value = CONSOLE_CLI_ENABLED },
  },
  { CONFIG_TYPE_UINT16, "timeout",
    .description = "Wait [ms] timeout to open UART console",
    .uint16_type = { .value = &console_config.timeout, .default_value = CONSOLE_CLI_TIMEOUT },
  },
  {}
};
