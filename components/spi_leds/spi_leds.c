#include <spi_leds.h>
#include "spi_leds.h"
#include "apa102.h"
#include "p9813.h"

#include <logging.h>

#include <esp_err.h>
#include <stdlib.h>
#include <string.h>

enum spi_leds_interface spi_leds_interface_for_protocol(enum spi_leds_protocol protocol)
{
  switch (protocol) {
    case SPI_LEDS_PROTOCOL_APA102:
    case SPI_LEDS_PROTOCOL_P9813:
      return SPI_LEDS_INTERFACE_SPI;

    case SPI_LEDS_PROTOCOL_WS2812B:
    case SPI_LEDS_PROTOCOL_SK6812_GRBW:
      return SPI_LEDS_INTERFACE_UART;

    default:
      // unknown
      return 0;
  }
}

static inline bool spi_led_color_active (struct spi_led_color color)
{
  return (color.r) || (color.g) || (color.b);
}

int spi_leds_init(struct spi_leds *spi_leds, const struct spi_leds_options *options)
{
  spi_leds->options = *options;
  spi_leds->active = 0;

  switch(options->protocol) {
    case SPI_LEDS_PROTOCOL_APA102:
      return spi_leds_init_apa102(&spi_leds->state.apa102, options);

    case SPI_LEDS_PROTOCOL_P9813:
      return spi_leds_init_p9813(&spi_leds->state.p9813, options);

    case SPI_LEDS_PROTOCOL_WS2812B:
      return spi_leds_init_ws2812b(&spi_leds->state.ws2812b, options);

    case SPI_LEDS_PROTOCOL_SK6812_GRBW:
      return spi_leds_init_sk6812grbw(&spi_leds->state.sk6812grbw, options);

    default:
      LOG_ERROR("unknown protocol=%#x", options->protocol);
      return -1;
  }
}

int spi_leds_new(struct spi_leds **spi_ledsp, const struct spi_leds_options *options)
{
  struct spi_leds *spi_leds;
  int err;

  if (!(spi_leds = calloc(1, sizeof(*spi_leds)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = spi_leds_init(spi_leds, options))) {
    LOG_ERROR("spi_leds_init");
    free(spi_leds);
    return -1;
  }

  *spi_ledsp = spi_leds;

  return 0;
}

unsigned spi_leds_count(struct spi_leds *spi_leds)
{
  return spi_leds->options.count;
}

unsigned spi_leds_active(struct spi_leds *spi_leds)
{
  unsigned active = 0;

  if (spi_leds->active) {
    switch(spi_leds->options.protocol) {
      case SPI_LEDS_PROTOCOL_APA102:
        active = apa102_count_active(&spi_leds->state.apa102, spi_leds->options.count);
        break;

      case SPI_LEDS_PROTOCOL_P9813:
        active = p9813_count_active(&spi_leds->state.p9813, spi_leds->options.count);
        break;

      case SPI_LEDS_PROTOCOL_WS2812B:
        active = ws2812b_count_active(&spi_leds->state.ws2812b, spi_leds->options.count);
        break;

      case SPI_LEDS_PROTOCOL_SK6812_GRBW:
        active = sk6812grbw_count_active(&spi_leds->state.sk6812grbw, spi_leds->options.count);
        break;

      default:
        LOG_ERROR("unknown protocol=%#x", spi_leds->options.protocol);
        abort();
    }

    if (!active) {
      spi_leds->active = false;
    }
  }

  return active;
}

int spi_leds_set(struct spi_leds *spi_leds, unsigned index, struct spi_led_color color)
{
  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", index, color.parameters.parameter, color.r, color.g, color.b);

  if (index >= spi_leds->options.count) {
    LOG_WARN("index %u >= count %u", index, spi_leds->options.count);
    return -1;
  }

  if (spi_led_color_active(color)) {
    spi_leds->active = true;
  }

  switch(spi_leds->options.protocol) {
    case SPI_LEDS_PROTOCOL_APA102:
      apa102_set_frame(&spi_leds->state.apa102, index, color);
      return 0;

    case SPI_LEDS_PROTOCOL_P9813:
      p9813_set_frame(&spi_leds->state.p9813, index, color);
      return 0;

    case SPI_LEDS_PROTOCOL_WS2812B:
      ws2812b_set_frame(&spi_leds->state.ws2812b, index, color);
      return 0;

    case SPI_LEDS_PROTOCOL_SK6812_GRBW:
      sk6812grbw_set_frame(&spi_leds->state.sk6812grbw, index, color);
      return 0;

    default:
      LOG_ERROR("unknown protocol=%#x", spi_leds->options.protocol);
      return -1;
  }
}

int spi_leds_set_all(struct spi_leds *spi_leds, struct spi_led_color color)
{
  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", spi_leds->options.count, color.parameters.parameter, color.r, color.g, color.b);

  if (spi_led_color_active(color)) {
    spi_leds->active = true;
  } else {
    spi_leds->active = 0;
  }

  switch(spi_leds->options.protocol) {
    case SPI_LEDS_PROTOCOL_APA102:
      apa102_set_frames(&spi_leds->state.apa102, spi_leds->options.count, color);
      return 0;

    case SPI_LEDS_PROTOCOL_P9813:
      p9813_set_frames(&spi_leds->state.p9813, spi_leds->options.count, color);
      return 0;

    case SPI_LEDS_PROTOCOL_WS2812B:
      ws2812b_set_frames(&spi_leds->state.ws2812b, spi_leds->options.count, color);
      return 0;

    case SPI_LEDS_PROTOCOL_SK6812_GRBW:
      sk6812grbw_set_frames(&spi_leds->state.sk6812grbw, spi_leds->options.count, color);
      return 0;

    default:
      LOG_ERROR("unknown protocol=%#x", spi_leds->options.protocol);
      return -1;
  }
}

int spi_leds_tx(struct spi_leds *spi_leds)
{
  switch(spi_leds->options.protocol) {
    case SPI_LEDS_PROTOCOL_APA102:
      spi_leds_tx_apa102(&spi_leds->state.apa102, &spi_leds->options);
      return 0;

    case SPI_LEDS_PROTOCOL_P9813:
      spi_leds_tx_p9813(&spi_leds->state.p9813, &spi_leds->options);
      return 0;

    case SPI_LEDS_PROTOCOL_WS2812B:
      spi_leds_tx_ws2812b(&spi_leds->state.ws2812b, &spi_leds->options);
      return 0;

    case SPI_LEDS_PROTOCOL_SK6812_GRBW:
      spi_leds_tx_sk6812grbw(&spi_leds->state.sk6812grbw, &spi_leds->options);
      return 0;

    default:
      LOG_ERROR("unknown protocol=%#x", spi_leds->options.protocol);
      return -1;
  }
}
