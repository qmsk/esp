#include "ws2812b.h"
#include "../leds.h"
#include "../interfaces/i2s.h"
#include "../interfaces/uart.h"

struct leds_protocol_type leds_protocol_ws2812b = {
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode  = LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL,
  .i2s_interface_func  = LEDS_INTERFACE_I2S_FUNC(i2s_mode_24bit_4x4, leds_protocol_ws2812b_i2s_out),
#endif
#if CONFIG_LEDS_UART_ENABLED
  .uart_interface_mode = LEDS_INTERFACE_UART_MODE_24B3I7_0U4_80U,
  .uart_interface_func = LEDS_INTERFACE_UART_FUNC(uart_mode_24B3I7, leds_protocol_ws2812b_uart_out),
#endif
  .parameter_type     = LEDS_PARAMETER_NONE,
  .power_mode         = LEDS_POWER_RGB,
};
