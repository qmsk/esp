#include "dmx.h"
#include "dmx_config.h"

#include <artnet.h>
#include <gpio.h>
#include <dmx_uart.h>

struct dmx_uart_config dmx_uart_config;
struct dmx_input_config dmx_input_config = {};
struct dmx_output_config dmx_output_configs[DMX_OUTPUT_COUNT] = {};

const struct config_enum dmx_uart_enum[] = {
  { "",        -1      },
#if defined(UART_0) && CONFIG_ESP_CONSOLE_UART_NUM != 0
  { "UART0",      UART_0  },
#endif
#if defined(UART_0_SWAP) && CONFIG_ESP_CONSOLE_UART_NUM != 0
  { "UART0_SWAP", UART_0_SWAP  }, // ESP8266 specialty
#endif
#if defined(UART_1) && CONFIG_ESP_CONSOLE_UART_NUM != 1
  { "UART1",      UART_1  },
#endif
#if defined(UART_2) && CONFIG_ESP_CONSOLE_UART_NUM != 2
  { "UART2",      UART_2  },
#endif
 {}
};

#if defined(UART_2) && CONFIG_ESP_CONSOLE_UART_NUM != 2
# define DMX_UART_CONFIG_PORT_DEFAULT_VALUE UART_2
#elif defined(UART_1) && CONFIG_ESP_CONSOLE_UART_NUM != 1
# define DMX_UART_CONFIG_PORT_DEFAULT_VALUE UART_1
#elif defined(UART_0_SWAP) && CONFIG_ESP_CONSOLE_UART_NUM != 0
# define DMX_UART_CONFIG_PORT_DEFAULT_VALUE UART_0_SWAP
#elif defined(UART_0) && CONFIG_ESP_CONSOLE_UART_NUM != 0
# define DMX_UART_CONFIG_PORT_DEFAULT_VALUE UART_0
#else
# define DMX_UART_CONFIG_PORT_DEFAULT_VALUE -1
#endif

const struct config_enum dmx_gpio_mode_enum[] = {
  { "",     DMX_GPIO_MODE_DISABLED  },
  { "LOW",  DMX_GPIO_MODE_LOW       },
  { "HIGH", DMX_GPIO_MODE_HIGH      },
  {}
};

const struct configtab dmx_uart_configtab[] = {
  { CONFIG_TYPE_ENUM, "port",
    .description = "Select host peripherial for UART interface.",
    .enum_type = { .value = &dmx_uart_config.port, .values = dmx_uart_enum, .default_value = DMX_UART_CONFIG_PORT_DEFAULT_VALUE },
  },
  {}
};

const struct configtab dmx_input_configtab[] = {
 { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &dmx_input_config.enabled },
 },
 { CONFIG_TYPE_UINT16, "mtbp_min",
    .description = (
      "Minimum mark-time-between-packets (us, approximately).\n"
      "Values ~128us or above are recommended. Default 0 -> 128us\n"
      "Values below 32us will drastically increase DMX input -> UART RX interrupt overhead, and may cause WiFi connections to fail."
      "DMX input may fail with UART RX break desynchronization errors if this is too high.\n"
    ),
    .uint16_type = { .value = &dmx_input_config.mtbp_min, .max = DMX_UART_MTBP_MAX },
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

const struct configtab *dmx_output_configtabs[DMX_OUTPUT_COUNT] = {
  dmx_output_configtab0,
  dmx_output_configtab1,
};
