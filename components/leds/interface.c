#include <leds.h>
#include "leds.h"
#include "interface.h"
#include "stats.h"

#include <logging.h>

int leds_interface_init(union leds_interface_state *interface, const struct leds_protocol_type *protocol_type, const struct leds_options *options)
{
  int err;
  
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      if (!protocol_type->spi_interface_mode) {
        LOG_ERROR("unsupported interface=SPI for protocol=%d", options->protocol);
        return -1;

      } else if ((err = leds_interface_spi_init(&interface->spi, &options->spi, protocol_type->spi_interface_mode, protocol_type->spi_interface_func))) {
        LOG_ERROR("leds_interface_spi_init");
        return err;
      }

      break;
  #endif


  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      if (!protocol_type->uart_interface_mode) {
        LOG_ERROR("unsupported interface=UART for protocol=%d", options->protocol);
        return -1;

      } else if ((err = leds_interface_uart_init(&interface->uart, &options->uart, protocol_type->uart_interface_mode, protocol_type->uart_interface_func))) {
        LOG_ERROR("leds_interface_uart_init");
        return err;
      }

      break;
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
  # if LEDS_I2S_INTERFACE_COUNT > 0
    case LEDS_INTERFACE_I2S0:
  # endif
  # if LEDS_I2S_INTERFACE_COUNT > 1
    case LEDS_INTERFACE_I2S1:
  # endif
      if (!protocol_type->i2s_interface_mode) {
        LOG_ERROR("unsupported interface=I2S for protocol=%d", options->protocol);
        return -1;

      } else if ((err = leds_interface_i2s_init(&interface->i2s, &options->i2s, protocol_type->i2s_interface_mode, protocol_type->i2s_interface_func, leds_interface_i2s_stats(options->interface)))) {
        LOG_ERROR("leds_interface_i2s_init");
        return err;
      }

      break;
  #endif

    default:
      LOG_ERROR("unsupported interface=%d", options->interface);
      return -1;
  }

  return 0;
}
