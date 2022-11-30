#include "p9813.h"
#include "../leds.h"
#include "../interfaces/spi.h"

struct leds_protocol_type leds_protocol_p9813 = {
#if CONFIG_LEDS_SPI_ENABLED
  .spi_interface_mode = LEDS_INTERFACE_SPI_MODE0_32BIT,
  .spi_interface_func = LEDS_INTERFACE_SPI_FUNC(spi_mode_32bit, leds_protocol_p9813_spi_out),
#endif
  .parameter_type     = LEDS_PARAMETER_NONE,
  .power_mode         = LEDS_POWER_RGB,
};
