#include <leds.h>
#include "leds.h"
#include "limit.h"

#include <logging.h>

#include <esp_err.h>
#include <stdlib.h>
#include <string.h>

enum leds_interface leds_interface_for_protocol(enum leds_protocol protocol)
{
  const struct leds_protocol_type *protocol_type = leds_protocol_type(protocol);

#if CONFIG_LEDS_I2S_ENABLED
  if (protocol_type->i2s_interface_mode) {
    return LEDS_INTERFACE_I2S;
  }
#endif

#if CONFIG_LEDS_UART_ENABLED
  if (protocol_type->uart_interface_mode) {
    return LEDS_INTERFACE_UART;
  }
#endif

  return LEDS_INTERFACE_NONE;
}

enum leds_parameter_type leds_parameter_type_for_protocol(enum leds_protocol protocol)
{
  return leds_protocol_type(protocol)->parameter_type;
}

uint8_t leds_parameter_default_for_protocol(enum leds_protocol protocol)
{
  switch (leds_parameter_type_for_protocol(protocol)) {
    case LEDS_PARAMETER_NONE:
      return 0;

    case LEDS_PARAMETER_DIMMER:
      return 255;

    case LEDS_PARAMETER_WHITE:
      return 0;

    default:
      // unknown
      return 0;
  }
}

enum leds_power_mode leds_power_mode_for_protocol(enum leds_protocol protocol)
{
  return leds_protocol_type(protocol)->power_mode;
}

int leds_init(struct leds *leds, const struct leds_options *options)
{
  int err;

  leds->options = *options;
  leds->active = 0;

  if (options->protocol < LEDS_PROTOCOLS_COUNT && leds_protocol_types[options->protocol]) {
    leds->protocol_type = leds_protocol_types[options->protocol];
  } else {
    LOG_ERROR("invalid protocol=%d", options->protocol);
    return -1;
  }

  if (!(leds->pixels = calloc(options->count, sizeof(*leds->pixels)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = leds_limit_init(&leds->limit, options->limit_groups, options->count))) {
    LOG_ERROR("leds_limit_init");
    return err;
  }

  if (!(leds->limit_groups_status = calloc(leds->limit.group_count, sizeof(*leds->limit_groups_status))) && leds->limit.group_count) {
    LOG_ERROR("calloc[limit_groups_status]");
    return err;
  }

  return leds->protocol_type->init(&leds->interface, options);
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

static bool leds_color_active (struct leds_color color, enum leds_parameter_type parameter_type)
{
  switch (parameter_type) {
    case LEDS_PARAMETER_NONE:
      return color.r || color.g || color.b;

    case LEDS_PARAMETER_DIMMER:
      return (color.r || color.g || color.b) && color.dimmer;

    case LEDS_PARAMETER_WHITE:
      return color.r || color.g || color.b || color.white;

    default:
      LOG_FATAL("invalid parameter_type=%u", parameter_type);
  }
}

int leds_clear_all(struct leds *leds)
{
  struct leds_color color = {}; // all off

  leds->active = false;

  for (unsigned i = 0; i < leds->options.count; i++) {
    leds->pixels[i] = color;
  }

  return 0;
}

int leds_set(struct leds *leds, unsigned index, struct leds_color color)
{
  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", index, color.parameter, color.r, color.g, color.b);

  if (index >= leds->options.count) {
    LOG_DEBUG("index %u >= count %u", index, leds->options.count);
    return -1;
  }

  if (leds_color_active(color, leds->protocol_type->parameter_type)) {
    leds->active = true;
  }

  leds->pixels[index] = color;

  return 0;
}

int leds_set_all(struct leds *leds, struct leds_color color)
{
  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", leds->options.count, color.parameter, color.r, color.g, color.b);

  if (leds_color_active(color, leds->protocol_type->parameter_type)) {
    leds->active = true;
  } else {
    leds->active = false;
  }

  for (unsigned i = 0; i < leds->options.count; i++) {
    leds->pixels[i] = color;
  }

  return 0;
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
    for (unsigned i = 0; i < leds->options.count; i++) {
      if (leds_color_active(leds->pixels[i], leds->protocol_type->parameter_type)) {
        active++;
      }
    }

    if (!active) {
      leds->active = false;
    }
  }

  return active;
}

static inline unsigned leds_power_rgb(struct leds_color color)
{
  return color.r + color.g + color.b;
}

static inline unsigned leds_power_rgba(struct leds_color color)
{
  // use brightness to 0..31 to not overflow a 32-bit uint for a 16-bit LEDS_COUNT_MAX
  return (color.r + color.g + color.b) * (color.dimmer >> 3);
}

static inline unsigned leds_power_rgbw(struct leds_color color)
{
  return color.r + color.g + color.b + color.white;
}

static inline unsigned leds_power_rgb2w(struct leds_color color)
{
  // white channel uses 200% power
  return color.r + color.g + color.b + (2 * color.white);
}

static unsigned leds_count_power(struct leds *leds, unsigned index, unsigned count)
{
  unsigned power = 0;

  for (unsigned i = index; i < i + count; i++) {
    switch (leds->protocol_type->power_mode) {
      case LEDS_POWER_NONE:
        break;

      case LEDS_POWER_RGB:
        power += leds_power_rgb(leds->pixels[i]);
        break;

      case LEDS_POWER_RGBA:
        power += leds_power_rgba(leds->pixels[i]);
        break;

      case LEDS_POWER_RGBW:
        power += leds_power_rgbw(leds->pixels[i]);
        break;

      case LEDS_POWER_RGB2W:
        power += leds_power_rgb2w(leds->pixels[i]);
        break;
    }
  }

  switch (leds->protocol_type->power_mode) {
    case LEDS_POWER_NONE:
      return 0;

    case LEDS_POWER_RGB:
      return power / (3 * 255);

    case LEDS_POWER_RGBA:
      return power / (3 * 255 * 31);

    case LEDS_POWER_RGBW:
      return power / (4 * 255);

    case LEDS_POWER_RGB2W:
      return power / (5 * 255);

    default:
      LOG_FATAL("invalid power_mode=%d for protocol=%d", leds->protocol_type->power_mode, leds->options.protocol);
  }
}

unsigned leds_count_total_power(struct leds *leds)
{
  return leds_count_power(leds, 0, leds->options.count);
}

static unsigned leds_count_group_power(struct leds *leds, unsigned group)
{
  unsigned count = leds->limit.group_size;
  unsigned index = leds->limit.group_size * group;

  return leds_count_power(leds, index, count);
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

  return leds->protocol_type->tx(&leds->interface, &leds->options, leds->pixels, &leds->limit);
}
