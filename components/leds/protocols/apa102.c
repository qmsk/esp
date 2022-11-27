#include "apa102.h"
#include "../leds.h"
#include "../interfaces/spi.h"

#include <logging.h>

static int leds_protocol_apa102_init(union leds_interface_state *interface, const struct leds_options *options)
{
  int err;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      if ((err = leds_interface_spi_init(&interface->spi, &options->spi, LEDS_PROTOCOL_APA102_INTERFACE_SPI_MODE, LEDS_INTERFACE_SPI_FUNC(spi_mode_32bit, leds_protocol_apa102_spi_out)))) {
        LOG_ERROR("leds_interface_spi_init");
        return err;
      }

      break;
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      if ((err = leds_interface_i2s_init(&interface->i2s, &options->i2s, LEDS_PROTOCOL_APA102_INTERFACE_I2S_MODE, LEDS_INTERFACE_I2S_FUNC(i2s_mode_32bit, leds_protocol_apa102_i2s_out)))) {
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

struct leds_protocol_type leds_protocol_apa102 = {
#if CONFIG_LEDS_SPI_ENABLED
  .spi_interface_mode = LEDS_PROTOCOL_APA102_INTERFACE_SPI_MODE,
#endif
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode = LEDS_PROTOCOL_APA102_INTERFACE_I2S_MODE,
#endif
  .parameter_type     = LEDS_PARAMETER_DIMMER,
  .power_mode         = LEDS_POWER_RGBA,

  .init               = &leds_protocol_apa102_init,
};
