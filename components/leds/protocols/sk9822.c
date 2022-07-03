#include "sk9822.h"
#include "../leds.h"
#include "../interfaces/i2s.h"

#include <logging.h>

#include <stdlib.h>

static void leds_protocol_sk9822_i2s_out(uint32_t buf[1], void *data, unsigned index, const struct leds_limit *limit)
{
  union sk9822_pixel *pixels = data;
  uint32_t xbgr = sk9822_pixel_limit(pixels[index], index, limit).xbgr;

  // 32-bit little-endian
  buf[0] = xbgr;
}

int leds_protocol_sk9822_init(struct leds_protocol_sk9822 *protocol, union leds_interface_state *interface, const struct leds_options *options)
{
  int err;

  if (!(protocol->pixels = calloc(options->count, sizeof(*protocol->pixels)))) {
    LOG_ERROR("malloc");
    return -1;
  }

  protocol->count = options->count;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      if ((err = leds_interface_i2s_init(&interface->i2s, &options->i2s, LEDS_INTERFACE_I2S_MODE_32BIT_BCK, protocol->count))) {
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

int leds_protocol_sk9822_tx(struct leds_protocol_sk9822 *protocol, union leds_interface_state *interface, const struct leds_options *options, const struct leds_limit *limit)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      // TODO: SK9822_START_FRAME_UINT32
      return leds_interface_i2s_tx(&interface->i2s, LEDS_INTERFACE_I2S_FUNC(i2s_mode_32bit, leds_protocol_sk9822_i2s_out), protocol->pixels, limit);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
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

unsigned leds_protocol_sk9822_count_power(struct leds_protocol_sk9822 *protocol, unsigned index, unsigned count)
{
  unsigned power = 0;

  for (unsigned i = index; i < index + count; i++) {
    power += sk9822_pixel_power(protocol->pixels[i]);
  }

  return power / SK9822_PIXEL_POWER_DIVISOR;
}
