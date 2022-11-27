#include "apa102.h"
#include "../leds.h"
#include "../interfaces/spi.h"
#include "../interfaces/i2s.h"

struct leds_protocol_type leds_protocol_apa102 = {
#if CONFIG_LEDS_SPI_ENABLED
  .spi_interface_mode = LEDS_INTERFACE_SPI_MODE3_32BIT,
  .spi_interface_func = LEDS_INTERFACE_SPI_FUNC(spi_mode_32bit, leds_protocol_apa102_spi_out),
#endif
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode = LEDS_INTERFACE_I2S_MODE_32BIT_BCK,
  .i2s_interface_func = LEDS_INTERFACE_I2S_FUNC(i2s_mode_32bit, leds_protocol_apa102_i2s_out),
#endif
  .parameter_type     = LEDS_PARAMETER_DIMMER,
  .power_mode         = LEDS_POWER_RGBA,
};
