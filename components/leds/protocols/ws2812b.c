#include "ws2812b.h"
#include "../leds.h"
#include "../interfaces/ws2812b.h"

#include <logging.h>

#include <stdlib.h>


int leds_protocol_ws2812b_init(union leds_interface_state *interface, struct leds_protocol_ws2812b *protocol, const struct leds_options *options)
{
  if (!(protocol->pixels = calloc(options->count, sizeof(*protocol->pixels)))) {
    LOG_ERROR("malloc");
    return -1;
  }

  return 0;
}

int leds_protocol_ws2812b_tx(union leds_interface_state *interface, struct leds_protocol_ws2812b *protocol, const struct leds_options *options)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      return leds_tx_uart_ws2812b(options, protocol->pixels, options->count);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_tx_i2s_ws2812b(options, protocol->pixels, options->count);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void leds_protocol_ws2812b_set_frame(struct leds_protocol_ws2812b *protocol, unsigned index, struct spi_led_color color)
{
  protocol->pixels[index] = (union ws2812b_pixel) {
    .b = color.b,
    .r = color.r,
    .g = color.g,
  };
}

void leds_protocol_ws2812b_set_frames(struct leds_protocol_ws2812b *protocol, unsigned count, struct spi_led_color color)
{
  for (unsigned index = 0; index < count; index++) {
    protocol->pixels[index] = (union ws2812b_pixel) {
      .b = color.b,
      .r = color.r,
      .g = color.g,
    };
  }
}

unsigned leds_protocol_ws2812b_count_active(struct leds_protocol_ws2812b *protocol, unsigned count)
{
  unsigned active = 0;

  for (unsigned index = 0; index < count; index++) {
    if (ws2812b_pixel_active(protocol->pixels[index])) {
      active++;
    }
  }

  return active;
}
