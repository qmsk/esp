#include "ws2811.h"
#include "leds.h"

#include <logging.h>

#include <stdlib.h>

int leds_init_ws2811(struct leds_protocol_ws2811 *protocol, const struct leds_options *options)
{
  if (!(protocol->pixels = calloc(options->count, sizeof(*protocol->pixels)))) {
    LOG_ERROR("malloc");
    return -1;
  }

  return 0;
}

int leds_tx_ws2811(struct leds_protocol_ws2811 *protocol, const struct leds_options *options)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      return leds_tx_uart_ws2811(options, protocol->pixels, options->count);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_tx_i2s_ws2811(options, protocol->pixels, options->count);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void ws2811_set_frame(struct leds_protocol_ws2811 *protocol, unsigned index, struct spi_led_color color)
{
  protocol->pixels[index] = (union ws2811_pixel) {
    .b = color.b,
    .g = color.g,
    .r = color.r,
  };
}

void ws2811_set_frames(struct leds_protocol_ws2811 *protocol, unsigned count, struct spi_led_color color)
{
  for (unsigned index = 0; index < count; index++) {
    protocol->pixels[index] = (union ws2811_pixel) {
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

static inline bool ws2811_pixel_active(const union ws2811_pixel pixel)
{
  return pixel.b || pixel.g || pixel.r;
}

unsigned ws2811_count_active(struct leds_protocol_ws2811 *protocol, unsigned count)
{
  unsigned active = 0;

  for (unsigned index = 0; index < count; index++) {
    if (ws2811_pixel_active(protocol->pixels[index])) {
      active++;
    }
  }

  return active;
}
