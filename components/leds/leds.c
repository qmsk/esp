#include <leds.h>
#include "leds.h"
#include "limit.h"

#include <logging.h>

#include <esp_err.h>
#include <stdlib.h>
#include <string.h>

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

  if ((err = leds_interface_init(&leds->interface, leds->protocol_type, &leds->options))) {
    LOG_ERROR("leds_interface_init");
    return err;
  }

  return 0;
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

enum leds_parameter_type leds_parameter_type(struct leds *leds)
{
  return leds->protocol_type->parameter_type;
}

uint8_t leds_parameter_default(struct leds *leds)
{
  return leds_parameter_default_for_type(leds->protocol_type->parameter_type);
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
    active = leds_colors_active(leds->pixels, leds->options.count, leds->protocol_type->parameter_type);

    if (!active) {
      leds->active = false;
    }
  }

  return active;
}

unsigned leds_count_total_power(struct leds *leds)
{
  return leds_power_total(leds->pixels, 0, leds->options.count, leds->protocol_type->power_mode);
}

static void leds_update_limit(struct leds *leds)
{
  unsigned total_power = 0;

  if (leds->limit.group_count && leds->options.limit_group) {
    for (unsigned group = 0; group < leds->limit.group_count; group++) {
      unsigned count = leds->limit.group_size;
      unsigned index = leds->limit.group_size * group;
      unsigned group_power = leds_power_total(leds->pixels, index, count, leds->protocol_type->power_mode);
      unsigned output_power = leds_limit_set_group(&leds->limit, group, leds->options.limit_group, group_power);

      total_power += output_power;

      LOG_DEBUG("group[%u] limit=%u power=%u -> output power=%u", group,
        leds->options.limit_group,
        group_power,
        output_power
      );

      leds->limit_groups_status[group] = (struct leds_limit_status) {
        .count  = count,
        .limit  = leds->options.limit_group,
        .power  = group_power,
        .output = output_power,
      };
    }
  } else {
    total_power = leds_power_total(leds->pixels, 0, leds->options.count, leds->protocol_type->power_mode);
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

  switch (leds->options.interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      return leds_interface_spi_tx(&leds->interface.spi, leds->pixels, leds->options.count, &leds->limit);
  #endif

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      return leds_interface_uart_tx(&leds->interface.uart, leds->pixels, leds->options.count, &leds->limit);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_interface_i2s_tx(&leds->interface.i2s, leds->pixels, leds->options.count, &leds->limit);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", leds->options.interface);
      return -1;
  }
}
