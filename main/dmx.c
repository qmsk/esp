#include "dmx.h"
#include "dmx_protocol.h"

#include <logging.h>
#include <uart1.h>

static const size_t DMX_TX_BUFFER_SIZE = 1 + 512; // fit one complete DMX frame
static const unsigned DMX_BREAK_US = 92, DMX_MARK_US = 12;

struct dmx_config {
  bool enabled;

  bool artnet_enabled;
  uint16_t artnet_universe;
} dmx_config;

const struct configtab dmx_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .value  = { .boolean = &dmx_config.enabled },
  },

  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .value  = { .boolean = &dmx_config.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &dmx_config.artnet_universe },
  },
  {}
};

struct dmx {
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
  if (!dmx_config.enabled) {
    LOG_ERROR("disabled");
    return -1;
  }

  for (int i = 0; i < len; i++) {
    LOG_DEBUG("[%03d] %02x", i, ((uint8_t *) data)[i]);
  }

  return dmx_send(&dmx, DMX_CMD_DIMMER, data, len);
}
