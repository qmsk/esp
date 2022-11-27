#include "ws2811.h"
#include "../leds.h"
#include "../interfaces/i2s.h"
#include "../interfaces/uart.h"

#include <logging.h>

int leds_protocol_ws2811_init(union leds_interface_state *interface, const struct leds_options *options)
{
  int err;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      if ((err = leds_interface_uart_init(&interface->uart, &options->uart, LEDS_PROTOCOL_WS2811_INTERFACE_UART_MODE, LEDS_INTERFACE_UART_FUNC(uart_mode_24B2I8, leds_protocol_ws2811_uart_out)))) {
        LOG_ERROR("leds_interface_uart_init");
        return err;
      }

      break;
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      if ((err = leds_interface_i2s_init(&interface->i2s, &options->i2s, LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE, options->count))) {
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

int leds_protocol_ws2811_tx(union leds_interface_state *interface, const struct leds_options *options, const struct leds_color *pixels, const struct leds_limit *limit)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      return leds_interface_uart_tx(&interface->uart, pixels, options->count, limit);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_interface_i2s_tx(&interface->i2s, LEDS_INTERFACE_I2S_FUNC(i2s_mode_24bit_4x4, leds_protocol_ws2811_i2s_out), pixels, limit);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
  }
}

struct leds_protocol_type leds_protocol_ws2811 = {
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode  = LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE,
#endif
#if CONFIG_LEDS_UART_ENABLED
  .uart_interface_mode = LEDS_PROTOCOL_WS2811_INTERFACE_UART_MODE,
#endif
  .parameter_type     = LEDS_PARAMETER_NONE,
  .power_mode         = LEDS_POWER_RGB,

  .init               = &leds_protocol_ws2811_init,
  .tx                 = &leds_protocol_ws2811_tx,
};
