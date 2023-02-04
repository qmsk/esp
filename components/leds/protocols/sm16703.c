#include "sm16703.h"
#include "../leds.h"
#include "../interfaces/i2s.h"

struct leds_protocol_type leds_protocol_sm16703 = {
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode  = LEDS_INTERFACE_I2S_MODE_24BIT_1U200_4X4_80UL,
  .i2s_interface_func  = LEDS_INTERFACE_I2S_FUNC(i2s_mode_24bit_4x4, leds_protocol_sm16703_i2s_out),
#endif
  .parameter_type     = LEDS_PARAMETER_NONE,
  .power_mode         = LEDS_POWER_RGB,
};
