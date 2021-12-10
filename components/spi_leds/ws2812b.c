#include "ws2812b.h"
#include "spi_leds.h"

#include <logging.h>

#include <stdlib.h>

int spi_leds_init_ws2812b(struct spi_leds_protocol_ws2812b *protocol, const struct spi_leds_options *options)
{
  if (!(protocol->pixels = calloc(options->count, sizeof(*protocol->pixels)))) {
    LOG_ERROR("malloc");
    return -1;
  }

  return 0;
}

int spi_leds_tx_ws2812b(struct spi_leds_protocol_ws2812b *protocol, const struct spi_leds_options *options)
{
  switch (options->interface) {
    case SPI_LEDS_INTERFACE_UART:
      return spi_leds_tx_uart_ws2812b(options, protocol->pixels, options->count);

    case SPI_LEDS_INTERFACE_I2S:
      return spi_leds_tx_i2s_ws2812b(options, protocol->pixels, options->count);

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void ws2812b_set_frame(struct spi_leds_protocol_ws2812b *protocol, unsigned index, struct spi_led_color color)
{
  protocol->pixels[index] = (union ws2812b_pixel) {
    .b = color.b,
    .r = color.r,
    .g = color.g,
  };
}

void ws2812b_set_frames(struct spi_leds_protocol_ws2812b *protocol, unsigned count, struct spi_led_color color)
{
  for (unsigned index = 0; index < count; index++) {
    protocol->pixels[index] = (union ws2812b_pixel) {
      .b = color.b,
      .r = color.r,
      .g = color.g,
    };
  }
}

static inline bool ws2812b_pixel_active(const union ws2812b_pixel pixel)
{
  return pixel.b || pixel.r || pixel.g;
}

unsigned ws2812b_count_active(struct spi_leds_protocol_ws2812b *protocol, unsigned count)
{
  unsigned active = 0;

  for (unsigned index = 0; index < count; index++) {
    if (ws2812b_pixel_active(protocol->pixels[index])) {
      active++;
    }
  }

  return active;
}
