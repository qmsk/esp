#include <dmx.h>
#include "dmx.h"

#include "activity_led.h"
#include <artnet.h>

#include <logging.h>
#include <uart1.h>

struct dmx_config {
  bool enabled;

  uint16_t gpio_pin;
  int gpio_mode;

  bool artnet_enabled;
  uint16_t artnet_universe;
} dmx_config = {
  .enabled            = false,
  .gpio_mode          = -1,
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
  /*
   * This should be a GPIO pin that's active low on boot, and used to drive the RS-485 transceiver's driver/output enable pin.
   * This allows supressing the UART1 bootloader debug output.
   */
  { CONFIG_TYPE_UINT16, "gpio_pin",
    .description = (
      "GPIO pin will be taken high to enable output once the UART1 TX output is safe."
    ),
    .uint16_type = { .value = &dmx_config.gpio_pin, .max = GPIO_OUT_PIN_MAX },
  },
  { CONFIG_TYPE_ENUM, "gpio_mode",
    .description = "Multiplex between multiple active-high/low GPIO-controlled outputs",
    .enum_type = { .value = &dmx_config.gpio_mode, .values = dmx_gpio_mode_enum },
  },

  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .bool_type = { .value = &dmx_config.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .description = "Output from universe (0-15) within [artnet] net/subnet.",
    .uint16_type = { .value = &dmx_config.artnet_universe, .max = ARTNET_UNIVERSE_MAX },
  },
  {}
};

struct uart1 *dmx_uart1;
struct gpio_out dmx_gpio_out;
struct dmx_output *dmx_output;

static int dmx_init_uart1 (const struct dmx_config *config)
{
  struct uart1_options uart1_options = {
    .clock_div   = DMX_BAUD_RATE,
    .data_bits   = DMX_DATA_BITS,
    .parity_bits = DMX_PARTITY_BITS,
    .stop_bits   = DMX_STOP_BITS,

    .tx_buffer_size = DMX_TX_BUFFER_SIZE,
  };
  int err;

  if ((err = uart1_new(&dmx_uart1, uart1_options))) {
    LOG_ERROR("uart1_new");
    return err;
  }

  return 0;
}

static int dmx_init_gpio(const struct dmx_config *config)
{
  int err;
  enum gpio_out_pins pins = 0;
  enum gpio_out_level level = 0;

  if (config->gpio_mode != -1) {
    level = config->gpio_mode;
    pins |= gpio_out_pin(config->gpio_pin);
  }

  LOG_INFO("pins=%04x level=%d", pins, level);

  if ((err = gpio_out_init(&dmx_gpio_out, pins, level))) {
    LOG_ERROR("gpio_out_init");
    return err;
  }

  return 0;
}

static int dmx_init_output(const struct dmx_config *config)
{
  struct dmx_output_options options = {
    .gpio_out       = &dmx_gpio_out,
    .gpio_out_pins  = gpio_out_pin(config->gpio_pin),
  };
  int err;

  LOG_INFO("uart1=%p gpio_out=%p gpio_out_pins=%04x", dmx_uart1, options.gpio_out, options.gpio_out_pins);

  if ((err = dmx_output_new(&dmx_output, dmx_uart1, options))) {
    LOG_ERROR("dmx_output_new");
    return err;
  }

  return 0;
}

int init_dmx()
{
  int err;

  if (!dmx_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = dmx_init_uart1(&dmx_config))) {
    LOG_ERROR("dmx_init_uart1");
    return err;
  }

  if ((err = dmx_init_gpio(&dmx_config))) {
    LOG_ERROR("dmx_init_gpio");
    return err;
  }

  if ((err = dmx_init_output(&dmx_config))) {
    LOG_ERROR("dmx_init_output");
    return err;
  }

  if ((err = dmx_output_enable(dmx_output))) {
    LOG_ERROR("dmx_output_enable");
    return err;
  }

  if (dmx_config.artnet_enabled) {
    if ((err = init_dmx_artnet(dmx_config.artnet_universe))) {
      LOG_ERROR("init_dmx_artnet");
      return err;
    }
  }

  return 0;
}

int output_dmx(void *data, size_t len)
{
  if (!dmx_output) {
    LOG_WARN("disabled");
    return -1;
  }

  for (int i = 0; i < len; i++) {
    LOG_DEBUG("[%03d] %02x", i, ((uint8_t *) data)[i]);
  }

  activity_led_event();

  return dmx_output_cmd(dmx_output, DMX_CMD_DIMMER, data, len);
}
