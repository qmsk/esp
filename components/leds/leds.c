#include <leds.h>
#include "leds.h"

#include <logging.h>

#include <esp_err.h>
#include <stdlib.h>
#include <string.h>

enum leds_interface leds_interface_for_protocol(enum leds_protocol protocol)
{
  switch (protocol) {
    case LEDS_PROTOCOL_APA102:
    case LEDS_PROTOCOL_P9813:
    #if LEDS_SPI_ENABLED
      return LEDS_INTERFACE_SPI;
    #else
      return LEDS_INTERFACE_NONE;
    #endif

    case LEDS_PROTOCOL_WS2812B:
    case LEDS_PROTOCOL_SK6812_GRBW:
    case LEDS_PROTOCOL_WS2811:
    #if LEDS_UART_ENABLED
      return LEDS_INTERFACE_UART;
    #else
      return LEDS_INTERFACE_NONE;
    #endif

    default:
      // unknown
      return 0;
  }
}

enum leds_color_parameter leds_color_parameter_for_protocol(enum leds_protocol protocol)
{
  switch (protocol) {
    case LEDS_PROTOCOL_APA102:
      return LEDS_COLOR_DIMMER;

    case LEDS_PROTOCOL_P9813:
      return LEDS_COLOR_NONE;

    case LEDS_PROTOCOL_WS2812B:
      return LEDS_COLOR_NONE;

    case LEDS_PROTOCOL_SK6812_GRBW:
      return LEDS_COLOR_WHITE;

    case LEDS_PROTOCOL_WS2811:
      return LEDS_COLOR_NONE;

    default:
      // unknown
      return 0;
  }
}

#if CONFIG_LEDS_SPI_ENABLED
  size_t leds_spi_buffer_for_protocol(enum leds_protocol protocol, unsigned count)
  {
    switch (protocol) {
      case LEDS_PROTOCOL_APA102:
        return leds_protocol_apa102_spi_buffer_size(count);

      case LEDS_PROTOCOL_P9813:
        return leds_protocol_p9813_spi_buffer_size(count);

      default:
        // unknown
        return 0;
    }
  }
#endif

size_t leds_i2s_buffer_for_protocol(enum leds_protocol protocol, unsigned count)
{
  switch (protocol) {
    case LEDS_PROTOCOL_WS2812B:
      return leds_protocol_ws2812b_i2s_buffer_size(count);

    case LEDS_PROTOCOL_WS2811:
      return leds_protocol_ws2811_i2s_buffer_size(count);

    case LEDS_PROTOCOL_SK6812_GRBW:
      return leds_protocol_sk6812grbw_i2s_buffer_size(count);

    default:
      // unknown
      return 0;
  }
}

uint8_t leds_default_color_parameter_for_protocol(enum leds_protocol protocol)
{
  switch (protocol) {
    case LEDS_PROTOCOL_APA102:
      return 255;

    case LEDS_PROTOCOL_P9813:
      return 0;

    case LEDS_PROTOCOL_WS2812B:
      return 0;

    case LEDS_PROTOCOL_SK6812_GRBW:
      return 0;

    case LEDS_PROTOCOL_WS2811:
      return 0;

    default:
      // unknown
      return 0;
  }
}

static inline bool leds_color_active (struct leds_color color)
{
  return (color.r) || (color.g) || (color.b);
}

int leds_init(struct leds *leds, const struct leds_options *options)
{
  leds->options = *options;
  leds->active = 0;

  switch(options->protocol) {
    case LEDS_PROTOCOL_APA102:
      return leds_protocol_apa102_init(&leds->interface, &leds->state.apa102, options);

    case LEDS_PROTOCOL_P9813:
      return leds_protocol_p9813_init(&leds->interface, &leds->state.p9813, options);

    case LEDS_PROTOCOL_WS2812B:
      return leds_protocol_ws2812b_init(&leds->interface, &leds->state.ws2812b, options);

    case LEDS_PROTOCOL_SK6812_GRBW:
      return leds_protocol_sk6812grbw_init(&leds->interface, &leds->state.sk6812grbw, options);

    case LEDS_PROTOCOL_WS2811:
      return leds_protocol_ws2811_init(&leds->interface, &leds->state.ws2811, options);

    default:
      LOG_ERROR("unknown protocol=%#x", options->protocol);
      return -1;
  }
}

int leds_new(struct leds **ledsp, const struct leds_options *options)
{
  struct leds *leds;
  int err;

  if (!(leds = calloc(1, sizeof(*leds)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = leds_init(leds, options))) {
    LOG_ERROR("leds_init");
    free(leds);
    return -1;
  }

  *ledsp = leds;

  return 0;
}

const struct leds_options *leds_options(struct leds *leds)
{
  return &leds->options;
}

enum leds_protocol leds_protocol(struct leds *leds)
{
  return leds->options.protocol;
}

enum leds_interface leds_interface(struct leds *leds)
{
  return leds->options.interface;
}

unsigned leds_count(struct leds *leds)
{
  return leds->options.count;
}

unsigned leds_active(struct leds *leds)
{
  unsigned active = 0;

  if (leds->active) {
    switch(leds->options.protocol) {
      case LEDS_PROTOCOL_APA102:
        active = leds_protocol_apa102_count_active(&leds->state.apa102, leds->options.count);
        break;

      case LEDS_PROTOCOL_P9813:
        active = leds_protocol_p9813_count_active(&leds->state.p9813, leds->options.count);
        break;

      case LEDS_PROTOCOL_WS2812B:
        active = leds_protocol_ws2812b_count_active(&leds->state.ws2812b, leds->options.count);
        break;

      case LEDS_PROTOCOL_SK6812_GRBW:
        active = leds_protocol_sk6812grbw_count_active(&leds->state.sk6812grbw, leds->options.count);
        break;

      case LEDS_PROTOCOL_WS2811:
        active = leds_protocol_ws2811_count_active(&leds->state.ws2811, leds->options.count);
        break;

      default:
        LOG_ERROR("unknown protocol=%#x", leds->options.protocol);
        abort();
    }

    if (!active) {
      leds->active = false;
    }
  }

  return active;
}

int leds_set(struct leds *leds, unsigned index, struct leds_color color)
{
  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", index, color.parameter, color.r, color.g, color.b);

  if (index >= leds->options.count) {
    LOG_DEBUG("index %u >= count %u", index, leds->options.count);
    return -1;
  }

  if (leds_color_active(color)) {
    leds->active = true;
  }

  switch(leds->options.protocol) {
    case LEDS_PROTOCOL_APA102:
      leds_protocol_apa102_set_frame(&leds->state.apa102, index, color);
      return 0;

    case LEDS_PROTOCOL_P9813:
      leds_protocol_p9813_set_frame(&leds->state.p9813, index, color);
      return 0;

    case LEDS_PROTOCOL_WS2812B:
      leds_protocol_ws2812b_set_frame(&leds->state.ws2812b, index, color);
      return 0;

    case LEDS_PROTOCOL_SK6812_GRBW:
      leds_protocol_sk6812grbw_set_frame(&leds->state.sk6812grbw, index, color);
      return 0;

    case LEDS_PROTOCOL_WS2811:
      leds_protocol_ws2811_set_frame(&leds->state.ws2811, index, color);
      return 0;

    default:
      LOG_ERROR("unknown protocol=%#x", leds->options.protocol);
      return -1;
  }
}

int leds_set_all(struct leds *leds, struct leds_color color)
{
  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", leds->options.count, color.parameter, color.r, color.g, color.b);

  if (leds_color_active(color)) {
    leds->active = true;
  } else {
    leds->active = 0;
  }

  switch(leds->options.protocol) {
    case LEDS_PROTOCOL_APA102:
      leds_protocol_apa102_set_frames(&leds->state.apa102, leds->options.count, color);
      return 0;

    case LEDS_PROTOCOL_P9813:
      leds_protocol_p9813_set_frames(&leds->state.p9813, leds->options.count, color);
      return 0;

    case LEDS_PROTOCOL_WS2812B:
      leds_protocol_ws2812b_set_frames(&leds->state.ws2812b, leds->options.count, color);
      return 0;

    case LEDS_PROTOCOL_SK6812_GRBW:
      leds_protocol_sk6812grbw_set_frames(&leds->state.sk6812grbw, leds->options.count, color);
      return 0;

    case LEDS_PROTOCOL_WS2811:
      leds_protocol_ws2811_set_frames(&leds->state.ws2811, leds->options.count, color);
      return 0;

    default:
      LOG_ERROR("unknown protocol=%#x", leds->options.protocol);
      return -1;
  }
}

int leds_set_format(struct leds *leds, enum leds_format format, void *data, size_t len, struct leds_format_params params)
{
  if (params.count == 0) {
    params.count = leds->options.count;
  }

  if (params.segment == 0) {
    params.segment = 1;
  }

  if (params.offset > leds->options.count) {
    LOG_DEBUG("offset=%u is over options.count=%u", params.offset, leds->options.count);
    params.count = 0;
  } else if (params.offset + params.count > leds->options.count) {
    LOG_DEBUG("offset=%u + count=%u is over options.count=%u", params.offset, params.count, leds->options.count);
    params.count = leds->options.count - params.offset;
  }

  switch(format) {
    case LEDS_FORMAT_RGB:
      leds_set_format_rgb(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_BGR:
      leds_set_format_bgr(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_GRB:
      leds_set_format_grb(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_RGBA:
      leds_set_format_rgba(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_RGBW:
      leds_set_format_rgbw(leds, data, len, params);
      return 0;

    default:
      LOG_ERROR("unknown format=%#x", format);
      return -1;
  }
}

int leds_tx(struct leds *leds)
{
  switch(leds->options.protocol) {
    case LEDS_PROTOCOL_APA102:
      return leds_protocol_apa102_tx(&leds->interface, &leds->state.apa102, &leds->options);

    case LEDS_PROTOCOL_P9813:
      return leds_protocol_p9813_tx(&leds->interface, &leds->state.p9813, &leds->options);

    case LEDS_PROTOCOL_WS2812B:
      return leds_protocol_ws2812b_tx(&leds->interface, &leds->state.ws2812b, &leds->options);

    case LEDS_PROTOCOL_SK6812_GRBW:
      return leds_protocol_sk6812grbw_tx(&leds->interface, &leds->state.sk6812grbw, &leds->options);

    case LEDS_PROTOCOL_WS2811:
      return leds_protocol_ws2811_tx(&leds->interface, &leds->state.ws2811, &leds->options);

    default:
      LOG_ERROR("unknown protocol=%#x", leds->options.protocol);
      return -1;
  }
}
