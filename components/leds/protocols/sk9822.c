#include "sk9822.h"
#include "../leds.h"
#include "../interfaces/i2s.h"

#include <logging.h>

static int leds_protocol_sk9822_init(union leds_interface_state *interface, const struct leds_options *options)
{
  int err;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      if ((err = leds_interface_spi_init(&interface->spi, &options->spi, LEDS_PROTOCOL_SK9822_INTERFACE_SPI_MODE, LEDS_INTERFACE_SPI_FUNC(spi_mode_32bit, leds_protocol_sk9822_spi_out)))) {
        LOG_ERROR("leds_interface_spi_init");
        return err;
      }

      break;
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      if ((err = leds_interface_i2s_init(&interface->i2s, &options->i2s, LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE, LEDS_INTERFACE_I2S_FUNC(i2s_mode_32bit, leds_protocol_sk9822_i2s_out)))) {
        LOG_ERROR("leds_interface_i2s_init");
        return err;
      }

      break;
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
  }

  return 0;
}

static int leds_protocol_sk9822_tx(union leds_interface_state *interface, const struct leds_options *options, const struct leds_color *pixels, const struct leds_limit *limit)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      return leds_interface_spi_tx(&interface->spi, pixels, options->count, limit);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_interface_i2s_tx(&interface->i2s, pixels, options->count, limit);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
  }
}

struct leds_protocol_type leds_protocol_sk9822 = {
#if CONFIG_LEDS_SPI_ENABLED
  .spi_interface_mode = LEDS_PROTOCOL_SK9822_INTERFACE_SPI_MODE,
#endif
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode = LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE,
#endif
  .parameter_type     = LEDS_PARAMETER_DIMMER,
  .power_mode         = LEDS_POWER_RGBA,

  .init               = &leds_protocol_sk9822_init,
  .tx                 = &leds_protocol_sk9822_tx,
};
