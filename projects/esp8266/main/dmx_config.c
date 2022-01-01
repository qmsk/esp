#include "dmx.h"
#include "dmx_config.h"

#include <artnet.h>
#include <gpio_out.h>
#include <uart.h>

#include <sdkconfig.h>

struct dmx_config dmx_config = {
  .uart   = -1,
};

struct dmx_output_config dmx_output_configs[DMX_OUTPUT_COUNT] = {
  [0] = {
    .gpio_mode         = -1,
  },
  [1] = {
    .gpio_mode        = -1,
  },
};

const struct config_enum dmx_uart_enum[] = {
 { "OFF",   -1              },
#if CONFIG_ESP_CONSOLE_UART_NUM != 0
 { "UART0", UART_0          },
#endif
#if CONFIG_ESP_CONSOLE_UART_NUM != 1
 { "UART1", UART_1          },
#endif
 {}
};

const struct config_enum dmx_gpio_mode_enum[] = {
 { "OFF",  -1              },
 { "HIGH", GPIO_OUT_HIGH   },
 { "LOW",  GPIO_OUT_LOW    },
 {}
};

const struct configtab dmx_configtab[] = {
 { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &dmx_config.enabled },
 },
 { CONFIG_TYPE_ENUM, "uart",
   .description = "Multiplex between multiple active-high/low GPIO-controlled outputs",
   .enum_type = { .value = &dmx_config.uart, .values = dmx_uart_enum },
 },
 { CONFIG_TYPE_BOOL, "input_enabled",
    .description = "Start DMX input on UART.",
    .bool_type = { .value = &dmx_config.input_enabled },
 },
 { CONFIG_TYPE_BOOL, "input_artnet_enabled",
    .description = "Configure Art-NET input port.",
    .bool_type = { .value = &dmx_config.input_artnet_enabled },
 },
 { CONFIG_TYPE_UINT16, "input_artnet_universe",
    .description = "Input to universe (0-15) within [artnet] net/subnet.",
    .uint16_type = { .value = &dmx_config.input_artnet_universe, .max = ARTNET_UNIVERSE_MAX },
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
