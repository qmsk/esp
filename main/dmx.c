#include "dmx.h"
#include "dmx_protocol.h"

#include "activity_led.h"
#include <artnet.h>

#include <driver/gpio.h>
#include <esp_err.h>
#include <logging.h>
#include <uart1.h>

#define DMX_OUTUT_ENABLE_GPIO_MAX GPIO_NUM_16

static const size_t DMX_TX_BUFFER_SIZE = 1 + 512; // fit one complete DMX frame
static const unsigned DMX_BREAK_US = 92, DMX_MARK_US = 12;

struct dmx_config {
  bool enabled;
  uint16_t output_enable_gpio;

  bool artnet_enabled;
  uint16_t artnet_universe;
} dmx_config = {
  .enabled            = false,
  .output_enable_gpio = GPIO_NUM_5,
};

const struct configtab dmx_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &dmx_config.enabled },
  },
  /*
   * This should be a GPIO pin that's active low on boot, and used to drive the RS-485 transceiver's driver/output enable pin.
   * This allows supressing the UART1 bootloader debug output.
   */
  { CONFIG_TYPE_UINT16, "output_enable_gpio",
    .description = (
      "GPIO pin will be taken high to enable output once the UART1 TX output is safe."
    ),
    .uint16_type = { .value = &dmx_config.output_enable_gpio, .max = DMX_OUTUT_ENABLE_GPIO_MAX },
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

struct dmx {
  gpio_num_t output_enable_gpio;
  struct uart1 *uart1;
} dmx;

int dmx_init (struct dmx *dmx, const struct dmx_config *config)
{
  struct uart1_options uart1_options = {
    .clock_div   = UART1_BAUD_250000,
    .data_bits   = UART1_DATA_BITS_8,
    .parity_bits = UART1_PARTIY_DISABLE,
    .stop_bits   = UART1_STOP_BITS_2,

    .tx_buffer_size = DMX_TX_BUFFER_SIZE,
  };
  int err;

  if ((err = uart1_new(&dmx->uart1, uart1_options))) {
    LOG_ERROR("uart1_new");
    return err;
  }

  return 0;
}

static int dmx_init_gpio(struct dmx *dmx, const struct dmx_config *config)
{
  esp_err_t err;

  dmx->output_enable_gpio = config->output_enable_gpio;

  if ((err = gpio_set_direction(dmx->output_enable_gpio, GPIO_MODE_OUTPUT))) {
    LOG_ERROR("gpio_set_direction: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = gpio_set_pull_mode(dmx->output_enable_gpio, GPIO_PULLDOWN_ONLY))) {
    LOG_ERROR("gpio_set_pull_mode: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = gpio_set_level(dmx->output_enable_gpio, 0))) {
    LOG_ERROR("gpio_set_level: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int dmx_output_enable (struct dmx *dmx)
{
  esp_err_t err;

  if ((err = gpio_set_level(dmx->output_enable_gpio, 1))) {
    LOG_ERROR("gpio_set_level: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int dmx_send (struct dmx *dmx, enum dmx_cmd cmd, void *data, size_t len)
{
  int err;

  // send break/mark per spec minimums for transmit; actual timings will vary, these are minimums
  if ((err = uart1_break(dmx->uart1, DMX_BREAK_US, DMX_MARK_US))) {
    LOG_ERROR("uart1_break");
    return err;
  }

  if ((err = uart1_putc(dmx->uart1, cmd)) < 0) {
    LOG_ERROR("uart1_putc");
    return err;
  }

  for (uint8_t *ptr = data; len > 0; ) {
    ssize_t write;

    if ((write = uart1_write(dmx->uart1, ptr, len)) < 0) {
      LOG_ERROR("uart1_write");
      return write;
    }

    ptr += write;
    len -= write;
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

  if ((err = dmx_init(&dmx, &dmx_config))) {
    LOG_ERROR("dmx_init");
    return err;
  }

  if ((err = dmx_init_gpio(&dmx, &dmx_config))) {
    LOG_ERROR("dmx_init_gpio");
    return err;
  }

  if (dmx_config.artnet_enabled) {
    if ((err = init_dmx_artnet(dmx_config.artnet_universe))) {
      LOG_ERROR("init_dmx_artnet");
      return err;
    }
  }

  if ((err = dmx_output_enable(&dmx))) {
    LOG_ERROR("dmx_output_enable");
    return err;
  }

  return 0;
}

int output_dmx(void *data, size_t len)
{
  if (!dmx_config.enabled) {
    LOG_ERROR("disabled");
    return -1;
  }

  for (int i = 0; i < len; i++) {
    LOG_DEBUG("[%03d] %02x", i, ((uint8_t *) data)[i]);
  }

  activity_led_event();

  return dmx_send(&dmx, DMX_CMD_DIMMER, data, len);
}
