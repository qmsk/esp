#include <leds.h>
#include "leds.h"
#include "limit.h"

#include <logging.h>

#include <esp_err.h>
#include <stdlib.h>
#include <string.h>

enum leds_interface leds_interface_for_protocol(enum leds_protocol protocol)
{
  switch (protocol) {
    case LEDS_PROTOCOL_NONE:
      return LEDS_INTERFACE_NONE;

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

    case LEDS_PROTOCOL_SK9822:
      return LEDS_INTERFACE_I2S;

    default:
      // unknown
      return 0;
  }
}

enum leds_color_parameter leds_color_parameter_for_protocol(enum leds_protocol protocol)
{
  switch (protocol) {
    case LEDS_PROTOCOL_NONE:
      return LEDS_COLOR_NONE;

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

    case LEDS_PROTOCOL_SK9822:
      return LEDS_COLOR_DIMMER;

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

uint8_t leds_default_color_parameter_for_protocol(enum leds_protocol protocol)
{
  switch (protocol) {
    case LEDS_PROTOCOL_NONE:
      return 0;

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

    case LEDS_PROTOCOL_SK9822:
      return 255;

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
  int err;

  leds->options = *options;
  leds->active = 0;

  if ((err = leds_limit_init(&leds->limit, options->limit_groups, options->count))) {
    LOG_ERROR("leds_limit_init");
    return err;
  }

  if (!(leds->limit_groups_status = calloc(leds->limit.group_count, sizeof(*leds->limit_groups_status)))) {
    LOG_ERROR("calloc[limit_groups_status]");
    return err;
  }

  switch(options->protocol) {
    case LEDS_PROTOCOL_NONE:
      return 0;

    case LEDS_PROTOCOL_APA102:
      return leds_protocol_apa102_init(&leds->protocol.apa102, &leds->interface, options);

    case LEDS_PROTOCOL_P9813:
      return leds_protocol_p9813_init(&leds->protocol.p9813, &leds->interface, options);

    case LEDS_PROTOCOL_WS2812B:
      return leds_protocol_ws2812b_init(&leds->protocol.ws2812b, &leds->interface, options);

    case LEDS_PROTOCOL_SK6812_GRBW:
      return leds_protocol_sk6812grbw_init(&leds->protocol.sk6812grbw, &leds->interface, options);

    case LEDS_PROTOCOL_WS2811:
      return leds_protocol_ws2811_init(&leds->protocol.ws2811, &leds->interface, options);

    case LEDS_PROTOCOL_SK9822:
      return leds_protocol_sk9822_init(&leds->protocol.sk9822, &leds->interface, options);

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
    goto error;
  }

  *ledsp = leds;

  return 0;

error:
  free(leds);

  return err;
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

int leds_clear_all(struct leds *leds)
{
  struct leds_color color = {}; // all off

  leds->active = false;

  switch(leds->options.protocol) {
    case LEDS_PROTOCOL_NONE:
      return 0;

    case LEDS_PROTOCOL_APA102:
      leds_protocol_apa102_set_all(&leds->protocol.apa102, color);
      return 0;

    case LEDS_PROTOCOL_P9813:
      leds_protocol_p9813_set_all(&leds->protocol.p9813, color);
      return 0;

    case LEDS_PROTOCOL_WS2812B:
      leds_protocol_ws2812b_set_all(&leds->protocol.ws2812b, color);
      return 0;

    case LEDS_PROTOCOL_SK6812_GRBW:
      leds_protocol_sk6812grbw_set_all(&leds->protocol.sk6812grbw, color);
      return 0;

    case LEDS_PROTOCOL_WS2811:
      leds_protocol_ws2811_set_all(&leds->protocol.ws2811, color);
      return 0;

    case LEDS_PROTOCOL_SK9822:
      leds_protocol_sk9822_set_all(&leds->protocol.sk9822, color);
      return 0;

    default:
      LOG_ERROR("unknown protocol=%#x", leds->options.protocol);
      return -1;
  }
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
    case LEDS_PROTOCOL_NONE:
      return 0;

    case LEDS_PROTOCOL_APA102:
      leds_protocol_apa102_set(&leds->protocol.apa102, index, color);
      return 0;

    case LEDS_PROTOCOL_P9813:
      leds_protocol_p9813_set(&leds->protocol.p9813, index, color);
      return 0;

    case LEDS_PROTOCOL_WS2812B:
      leds_protocol_ws2812b_set(&leds->protocol.ws2812b, index, color);
      return 0;

    case LEDS_PROTOCOL_SK6812_GRBW:
      leds_protocol_sk6812grbw_set(&leds->protocol.sk6812grbw, index, color);
      return 0;

    case LEDS_PROTOCOL_WS2811:
      leds_protocol_ws2811_set(&leds->protocol.ws2811, index, color);
      return 0;

    case LEDS_PROTOCOL_SK9822:
      leds_protocol_sk9822_set(&leds->protocol.sk9822, index, color);
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
    leds->active = false;
  }

  switch(leds->options.protocol) {
    case LEDS_PROTOCOL_NONE:
      return 0;

    case LEDS_PROTOCOL_APA102:
      leds_protocol_apa102_set_all(&leds->protocol.apa102, color);
      return 0;

    case LEDS_PROTOCOL_P9813:
      leds_protocol_p9813_set_all(&leds->protocol.p9813, color);
      return 0;

    case LEDS_PROTOCOL_WS2812B:
      leds_protocol_ws2812b_set_all(&leds->protocol.ws2812b, color);
      return 0;

    case LEDS_PROTOCOL_SK6812_GRBW:
      leds_protocol_sk6812grbw_set_all(&leds->protocol.sk6812grbw, color);
      return 0;

    case LEDS_PROTOCOL_WS2811:
      leds_protocol_ws2811_set_all(&leds->protocol.ws2811, color);
      return 0;

    case LEDS_PROTOCOL_SK9822:
      leds_protocol_sk9822_set_all(&leds->protocol.sk9822, color);
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

unsigned leds_count_active(struct leds *leds)
{
  unsigned active = 0;

  if (leds->active) {
    switch(leds->options.protocol) {
      case LEDS_PROTOCOL_NONE:
        active = 0;
        break;

      case LEDS_PROTOCOL_APA102:
        active = leds_protocol_apa102_count_active(&leds->protocol.apa102);
        break;

      case LEDS_PROTOCOL_P9813:
        active = leds_protocol_p9813_count_active(&leds->protocol.p9813);
        break;

      case LEDS_PROTOCOL_WS2812B:
        active = leds_protocol_ws2812b_count_active(&leds->protocol.ws2812b);
        break;

      case LEDS_PROTOCOL_SK6812_GRBW:
        active = leds_protocol_sk6812grbw_count_active(&leds->protocol.sk6812grbw);
        break;

      case LEDS_PROTOCOL_WS2811:
        active = leds_protocol_ws2811_count_active(&leds->protocol.ws2811);
        break;

      case LEDS_PROTOCOL_SK9822:
        active = leds_protocol_sk9822_count_active(&leds->protocol.sk9822);
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

unsigned leds_count_total_power(struct leds *leds)
{
  switch(leds->options.protocol) {
    case LEDS_PROTOCOL_NONE:
      return 0;

    case LEDS_PROTOCOL_APA102:
      return leds_protocol_apa102_count_power(&leds->protocol.apa102, 0, leds->options.count);

    case LEDS_PROTOCOL_P9813:
      return leds_protocol_p9813_count_power(&leds->protocol.p9813, 0, leds->options.count);

    case LEDS_PROTOCOL_WS2812B:
      return leds_protocol_ws2812b_count_power(&leds->protocol.ws2812b, 0, leds->options.count);

    case LEDS_PROTOCOL_SK6812_GRBW:
      return leds_protocol_sk6812grbw_count_power(&leds->protocol.sk6812grbw, 0, leds->options.count);

    case LEDS_PROTOCOL_WS2811:
      return leds_protocol_ws2811_count_power(&leds->protocol.ws2811, 0, leds->options.count);

    case LEDS_PROTOCOL_SK9822:
      return leds_protocol_sk9822_count_power(&leds->protocol.sk9822, 0, leds->options.count);

    default:
      LOG_ERROR("unknown protocol=%#x", leds->options.protocol);
      return -1;
  }
}

static unsigned leds_count_group_power(struct leds *leds, unsigned group)
{
  unsigned count = leds->limit.group_size;
  unsigned index = leds->limit.group_size * group;

  switch(leds->options.protocol) {
    case LEDS_PROTOCOL_NONE:
      return 0;

    case LEDS_PROTOCOL_APA102:
      return leds_protocol_apa102_count_power(&leds->protocol.apa102, index, count);

    case LEDS_PROTOCOL_P9813:
      return leds_protocol_p9813_count_power(&leds->protocol.p9813, index, count);

    case LEDS_PROTOCOL_WS2812B:
      return leds_protocol_ws2812b_count_power(&leds->protocol.ws2812b, index, count);

    case LEDS_PROTOCOL_SK6812_GRBW:
      return leds_protocol_sk6812grbw_count_power(&leds->protocol.sk6812grbw, index, count);

    case LEDS_PROTOCOL_WS2811:
      return leds_protocol_ws2811_count_power(&leds->protocol.ws2811, index, count);

    case LEDS_PROTOCOL_SK9822:
      return leds_protocol_sk9822_count_power(&leds->protocol.sk9822, index, count);

    default:
      LOG_ERROR("unknown protocol=%#x", leds->options.protocol);
      return -1;
  }
}

void leds_update_limit(struct leds *leds)
{
  unsigned total_power = 0;

  if (leds->limit.group_count && leds->options.limit_group) {
    for (unsigned group = 0; group < leds->limit.group_count; group++) {
      unsigned group_power = leds_count_group_power(leds, group);
      unsigned output_power = leds_limit_set_group(&leds->limit, group, leds->options.limit_group, group_power);

      total_power += output_power;

      LOG_DEBUG("group[%u] limit=%u power=%u -> output power=%u", group,
        leds->options.limit_group,
        group_power,
        output_power
      );

      leds->limit_groups_status[group] = (struct leds_limit_status) {
        .count  = leds->limit.group_size,
        .limit  = leds->options.limit_group,
        .power  = group_power,
        .output = output_power,
      };
    }
  } else {
    total_power = leds_count_total_power(leds);
  }

  // apply total limit
  if (leds->options.limit_total) {
    unsigned output_power = leds_limit_set_total(&leds->limit, leds->options.limit_total, total_power);

    LOG_DEBUG("total limit=%u power=%u -> output power=%u",
      leds->options.limit_total,
      total_power,
      output_power
    );

    leds->limit_total_status = (struct leds_limit_status) {
      .count  = leds->options.count,
      .limit  = leds->options.limit_total,
      .power  = total_power,
      .output = output_power,
    };
  }
}

int leds_tx(struct leds *leds)
{
  leds_update_limit(leds);

  switch(leds->options.protocol) {
    case LEDS_PROTOCOL_NONE:
      return 0;

    case LEDS_PROTOCOL_APA102:
      // TODO: limit
      return leds_protocol_apa102_tx(&leds->protocol.apa102, &leds->interface, &leds->options);

    case LEDS_PROTOCOL_P9813:
      // TODO: limit
      return leds_protocol_p9813_tx(&leds->protocol.p9813, &leds->interface, &leds->options);

    case LEDS_PROTOCOL_WS2812B:
      return leds_protocol_ws2812b_tx(&leds->protocol.ws2812b, &leds->interface, &leds->options, &leds->limit);

    case LEDS_PROTOCOL_SK6812_GRBW:
      return leds_protocol_sk6812grbw_tx(&leds->protocol.sk6812grbw, &leds->interface, &leds->options, &leds->limit);

    case LEDS_PROTOCOL_WS2811:
      return leds_protocol_ws2811_tx(&leds->protocol.ws2811, &leds->interface, &leds->options, &leds->limit);

    case LEDS_PROTOCOL_SK9822:
      return leds_protocol_sk9822_tx(&leds->protocol.sk9822, &leds->interface, &leds->options, &leds->limit);

    default:
      LOG_ERROR("unknown protocol=%#x", leds->options.protocol);
      return -1;
  }
}
