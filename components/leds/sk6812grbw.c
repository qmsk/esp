#include "sk6812grbw.h"
#include "leds.h"

#include <logging.h>

#include <stdlib.h>

int leds_init_sk6812grbw(struct leds_protocol_sk6812grbw *protocol, const struct leds_options *options)
{
  if (!(protocol->pixels = calloc(options->count, sizeof(*protocol->pixels)))) {
    LOG_ERROR("malloc");
    return -1;
  }

  return 0;
}

int leds_tx_sk6812grbw(struct leds_protocol_sk6812grbw *protocol, const struct leds_options *options)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      return leds_tx_uart_sk6812grbw(options, protocol->pixels, options->count);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_tx_i2s_sk6812grbw(options, protocol->pixels, options->count);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void sk6812grbw_set_frame(struct leds_protocol_sk6812grbw *protocol, unsigned index, struct spi_led_color color)
{
  protocol->pixels[index] = (union sk6812grbw_pixel) {
    .w = color.white,
    .b = color.b,
    .r = color.r,
    .g = color.g,
  };
}

void sk6812grbw_set_frames(struct leds_protocol_sk6812grbw *protocol, unsigned count, struct spi_led_color color)
{
  for (unsigned index = 0; index < count; index++) {
    protocol->pixels[index] = (union sk6812grbw_pixel) {
      .w = color.white,
      .b = color.b,
      .r = color.r,
      .g = color.g,
    };
  }
}

static inline bool sk6812grbw_pixel_active(const union sk6812grbw_pixel pixel)
{
  return pixel.w || pixel.b || pixel.r || pixel.g;
}

unsigned sk6812grbw_count_active(struct leds_protocol_sk6812grbw *protocol, unsigned count)
{
  unsigned active = 0;

  for (unsigned index = 0; index < count; index++) {
    if (sk6812grbw_pixel_active(protocol->pixels[index])) {
      active++;
    }
  }

  return active;
}
