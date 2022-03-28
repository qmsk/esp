#include "sk9822.h"
#include "../leds.h"
#include "../interfaces/sk9822.h"

#include <logging.h>

#include <stdlib.h>

int leds_protocol_sk9822_init(struct leds_protocol_sk9822 *protocol, union leds_interface_state *interface, const struct leds_options *options)
{
  if (!(protocol->pixels = calloc(options->count, sizeof(*protocol->pixels)))) {
    LOG_ERROR("malloc");
    return -1;
  }

  protocol->count = options->count;

  return 0;
}

int leds_protocol_sk9822_tx(struct leds_protocol_sk9822 *protocol, union leds_interface_state *interface, const struct leds_options *options, struct leds_limit limit)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_tx_i2s_sk9822(&options->i2s, protocol->pixels, protocol->count, limit);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void leds_protocol_sk9822_set(struct leds_protocol_sk9822 *protocol, unsigned index, struct leds_color color)
{
  if (index < protocol->count) {
    protocol->pixels[index] = (union sk9822_pixel) {
      .global = SK9822_GLOBAL_BYTE(color.dimmer),
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

void leds_protocol_sk9822_set_all(struct leds_protocol_sk9822 *protocol, struct leds_color color)
{
  for (unsigned index = 0; index < protocol->count; index++) {
    protocol->pixels[index] = (union sk9822_pixel) {
      .global = SK9822_GLOBAL_BYTE(color.dimmer),
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

unsigned leds_protocol_sk9822_count_active(struct leds_protocol_sk9822 *protocol)
{
  unsigned active = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    if (sk9822_pixel_active(protocol->pixels[index])) {
      active++;
    }
  }

  return active;
}

unsigned leds_protocol_sk9822_count_total(struct leds_protocol_sk9822 *protocol)
{
  unsigned total = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    total += sk9822_pixel_total(protocol->pixels[index]);
  }

  return total / SK9822_PIXEL_TOTAL_DIVISOR;
}
