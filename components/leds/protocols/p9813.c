#include "p9813.h"
#include "../leds.h"
#include "../interfaces/spi.h"

#include <logging.h>

static int leds_protocol_p9813_init(union leds_interface_state *interface, const struct leds_options *options)
{
  int err;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      if ((err = leds_interface_spi_init(&interface->spi, &options->spi, LEDS_PROTOCOL_P9813_INTERFACE_SPI_MODE, LEDS_INTERFACE_SPI_FUNC(spi_mode_32bit, leds_protocol_p9813_spi_out)))) {
        LOG_ERROR("leds_interface_spi_init");
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

static int leds_protocol_p9813_tx(union leds_interface_state *interface, const struct leds_options *options, const struct leds_color *pixels, const struct leds_limit *limit)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      return leds_interface_spi_tx(&interface->spi, pixels, options->count, limit);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

struct leds_protocol_type leds_protocol_p9813 = {
#if CONFIG_LEDS_SPI_ENABLED
  .spi_interface_mode = LEDS_PROTOCOL_P9813_INTERFACE_SPI_MODE,
#endif
  .parameter_type     = LEDS_PARAMETER_NONE,
  .power_mode         = LEDS_POWER_RGB,

  .init               = &leds_protocol_p9813_init,
  .tx                 = &leds_protocol_p9813_tx,
};
