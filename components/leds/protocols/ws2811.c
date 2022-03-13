#include "ws2811.h"
#include "../leds.h"
#include "../interfaces/ws2811.h"

#include <logging.h>

#include <stdlib.h>

int leds_protocol_ws2811_init(union leds_interface_state *interface, struct leds_protocol_ws2811 *protocol, const struct leds_options *options)
{
  if (!(protocol->pixels = calloc(options->count, sizeof(*protocol->pixels)))) {
    LOG_ERROR("malloc");
    return -1;
  }

  protocol->count = options->count;

  return 0;
}

int leds_protocol_ws2811_tx(union leds_interface_state *interface, struct leds_protocol_ws2811 *protocol, const struct leds_options *options)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      return leds_tx_uart_ws2811(options, protocol->pixels, protocol->count);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_tx_i2s_ws2811(options, protocol->pixels, protocol->count);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void leds_protocol_ws2811_set(struct leds_protocol_ws2811 *protocol, unsigned index, struct leds_color color)
{
  if (index < protocol->count) {
    protocol->pixels[index] = (union ws2811_pixel) {
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

void leds_protocol_ws2811_set_all(struct leds_protocol_ws2811 *protocol, struct leds_color color)
{
  for (unsigned index = 0; index < protocol->count; index++) {
    protocol->pixels[index] = (union ws2811_pixel) {
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

unsigned leds_protocol_ws2811_count_active(struct leds_protocol_ws2811 *protocol)
{
  unsigned active = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    if (ws2811_pixel_active(protocol->pixels[index])) {
      active++;
    }
  }

  return active;
}

unsigned leds_protocol_ws2811_count_total(struct leds_protocol_ws2811 *protocol)
{
  unsigned total = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    total += ws2811_pixel_total(protocol->pixels[index]);
  }

  return total / WS2811_PIXEL_TOTAL_DIVISOR;
}
