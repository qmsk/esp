#include "sk6812grbw.h"
#include "../leds.h"
#include "../interfaces/sk6812grbw.h"

#include <logging.h>

#include <stdlib.h>

int leds_protocol_sk6812grbw_init(union leds_interface_state *interface, struct leds_protocol_sk6812grbw *protocol, const struct leds_options *options)
{
  if (!(protocol->pixels = calloc(options->count, sizeof(*protocol->pixels)))) {
    LOG_ERROR("malloc");
    return -1;
  }

  protocol->count = options->count;

  return 0;
}

int leds_protocol_sk6812grbw_tx(union leds_interface_state *interface, struct leds_protocol_sk6812grbw *protocol, const struct leds_options *options)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      return leds_tx_uart_sk6812grbw(options, protocol->pixels, protocol->count);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_tx_i2s_sk6812grbw(options, protocol->pixels, protocol->count);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void leds_protocol_sk6812grbw_set(struct leds_protocol_sk6812grbw *protocol, unsigned index, struct leds_color color)
{
  if (index < protocol->count) {
    protocol->pixels[index] = (union sk6812grbw_pixel) {
      .w = color.white,
      .b = color.b,
      .r = color.r,
      .g = color.g,
    };
  }
}

void leds_protocol_sk6812grbw_set_all(struct leds_protocol_sk6812grbw *protocol, struct leds_color color)
{
  for (unsigned index = 0; index < protocol->count; index++) {
    protocol->pixels[index] = (union sk6812grbw_pixel) {
      .w = color.white,
      .b = color.b,
      .r = color.r,
      .g = color.g,
    };
  }
}

unsigned leds_protocol_sk6812grbw_count_active(struct leds_protocol_sk6812grbw *protocol)
{
  unsigned active = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    if (sk6812grbw_pixel_active(protocol->pixels[index])) {
      active++;
    }
  }

  return active;
}

unsigned leds_protocol_sk6812grbw_count_total(struct leds_protocol_sk6812grbw *protocol)
{
  unsigned total = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    total += sk6812grbw_pixel_total(protocol->pixels[index]);
  }

  return total / SK6812_GRBW_PIXEL_TOTAL_DIVISOR;
}
