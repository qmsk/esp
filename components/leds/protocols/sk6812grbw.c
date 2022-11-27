#include "sk6812grbw.h"
#include "../leds.h"
#include "../interfaces/i2s.h"
#include "../interfaces/uart.h"

#include <logging.h>

#include <stdlib.h>

int leds_protocol_sk6812grbw_init(union leds_interface_state *interface, const struct leds_options *options)
{
  int err;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      if ((err = leds_interface_uart_init(&interface->uart, &options->uart, LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_UART_MODE, LEDS_INTERFACE_UART_FUNC(uart_mode_32B2I6, leds_protocol_sk6812grbw_uart_out)))) {
        LOG_ERROR("leds_interface_uart_init");
        return err;
      }

      break;
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      if ((err = leds_interface_i2s_init(&interface->i2s, &options->i2s, LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE, LEDS_INTERFACE_I2S_FUNC(i2s_mode_32bit_4x4, leds_protocol_sk6812grbw_i2s_out)))) {
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

struct leds_protocol_type leds_protocol_sk6812grbw = {
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode  = LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE,
#endif
#if CONFIG_LEDS_UART_ENABLED
  .uart_interface_mode = LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_UART_MODE,
#endif
  .parameter_type     = LEDS_PARAMETER_WHITE,
  .power_mode         = LEDS_POWER_RGB2W,

  .init               = &leds_protocol_sk6812grbw_init,
};
