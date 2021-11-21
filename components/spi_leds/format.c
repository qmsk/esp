#include "spi_leds.h"

#include <logging.h>

void spi_leds_set_format_rgb(struct spi_leds *spi_leds, uint8_t *data, size_t len, struct spi_leds_format_params params)
{
  uint8_t parameter = spi_leds_default_color_parameter_for_protocol(spi_leds->options.protocol);

  LOG_DEBUG("len=%u offset=%u count=%u", len, params.offset, params.count);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    spi_leds_set(spi_leds, params.offset + i, (struct spi_led_color) {
      .r = data[i * 3 + 0],
      .g = data[i * 3 + 1],
      .b = data[i * 3 + 2],

      .parameter = parameter,
    });
  }
}

void spi_leds_set_format_bgr(struct spi_leds *spi_leds, uint8_t *data, size_t len, struct spi_leds_format_params params)
{
  uint8_t parameter = spi_leds_default_color_parameter_for_protocol(spi_leds->options.protocol);

  LOG_DEBUG("len=%u offset=%u count=%u", len, params.offset, params.count);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    spi_leds_set(spi_leds, params.offset + i, (struct spi_led_color) {
      .b = data[i * 3 + 0],
      .g = data[i * 3 + 1],
      .r = data[i * 3 + 2],

      .parameter = parameter,
    });
  }
}

void spi_leds_set_format_grb(struct spi_leds *spi_leds, uint8_t *data, size_t len, struct spi_leds_format_params params)
{
  uint8_t parameter = spi_leds_default_color_parameter_for_protocol(spi_leds->options.protocol);

  LOG_DEBUG("len=%u offset=%u count=%u", len, params.offset, params.count);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    spi_leds_set(spi_leds, params.offset + i, (struct spi_led_color) {
      .g = data[i * 3 + 0],
      .r = data[i * 3 + 1],
      .b = data[i * 3 + 2],

      .parameter = parameter,
    });
  }
}

void spi_leds_set_format_rgba(struct spi_leds *spi_leds, uint8_t *data, size_t len, struct spi_leds_format_params params)
{
  enum spi_leds_color_parameter parameter = spi_leds_color_parameter_for_protocol(spi_leds->options.protocol);

  LOG_DEBUG("len=%u offset=%u count=%u", len, params.offset, params.count);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 4; i++) {
    spi_leds_set(spi_leds, params.offset + i, (struct spi_led_color) {
      .r = data[i * 4 + 0],
      .g = data[i * 4 + 1],
      .b = data[i * 4 + 2],

      .brightness = (parameter == SPI_LEDS_COLOR_BRIGHTNESS) ? data[i * 4 + 3] : 255,
    });
  }
}

void spi_leds_set_format_rgbw(struct spi_leds *spi_leds, uint8_t *data, size_t len, struct spi_leds_format_params params)
{
  enum spi_leds_color_parameter parameter = spi_leds_color_parameter_for_protocol(spi_leds->options.protocol);

  LOG_DEBUG("len=%u offset=%u count=%u", len, params.offset, params.count);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 4; i++) {
    spi_leds_set(spi_leds, params.offset + i, (struct spi_led_color) {
      .r = data[i * 4 + 0],
      .g = data[i * 4 + 1],
      .b = data[i * 4 + 2],

      .white = (parameter == SPI_LEDS_COLOR_WHITE) ? data[i * 4 + 3] : 255,
    });
  }
}
