#include <spi_leds.h>
#include "spi_leds.h"
#include "apa102.h"
#include "p9813.h"

#include <logging.h>

#include <esp_err.h>
#include <stdlib.h>
#include <string.h>

static inline bool spi_led_color_active (struct spi_led_color color)
{
  return (color.r) || (color.g) || (color.b);
}

int spi_leds_init(struct spi_leds *spi_leds, struct spi_master *spi_master, const struct spi_leds_options options)
{
  spi_leds->spi_master = spi_master;
  spi_leds->options = options;
  spi_leds->active = 0;

  switch(options.protocol) {
    case SPI_LEDS_PROTOCOL_APA102:
      spi_leds->spi_mode = APA102_SPI_MODE;

      return apa102_new_packet(&spi_leds->packet.apa102, &spi_leds->packet_size, options.count);

    case SPI_LEDS_PROTOCOL_P9813:
      spi_leds->spi_mode = P9813_SPI_MODE;

      return p9813_new_packet(&spi_leds->packet.p9813, &spi_leds->packet_size, options.count);

    default:
      LOG_ERROR("unknown protocol=%#x", options.protocol);
      return -1;
  }
}

int spi_leds_new(struct spi_leds **spi_ledsp, struct spi_master *spi_master, const struct spi_leds_options options)
{
  struct spi_leds *spi_leds;
  int err;

  if (!(spi_leds = calloc(1, sizeof(*spi_leds)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = spi_leds_init(spi_leds, spi_master, options))) {
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
        active = apa102_count_active(spi_leds->packet.apa102, spi_leds->options.count);
        break;

      case SPI_LEDS_PROTOCOL_P9813:
        active = p9813_count_active(spi_leds->packet.p9813, spi_leds->options.count);
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
      apa102_set_frame(spi_leds->packet.apa102, index, color);
      return 0;

    case SPI_LEDS_PROTOCOL_P9813:
      p9813_set_frame(spi_leds->packet.p9813, index, color);
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
      apa102_set_frames(spi_leds->packet.apa102, spi_leds->options.count, color);
      return 0;

    case SPI_LEDS_PROTOCOL_P9813:
      p9813_set_frames(spi_leds->packet.p9813, spi_leds->options.count, color);
      return 0;

    default:
      LOG_ERROR("unknown protocol=%#x", spi_leds->options.protocol);
      return -1;
  }
}

int spi_leds_tx(struct spi_leds *spi_leds)
{
  struct spi_write_options options = {
    .mode   = spi_leds->options.mode_bits | spi_leds->spi_mode | SPI_MODE_SET,
    .clock  = spi_leds->options.clock,
  };
  uint8_t *buf = spi_leds->packet.buf;
  unsigned len = spi_leds->packet_size;
  int ret, err;

  if ((err = spi_master_open(spi_leds->spi_master, options))) {
    LOG_ERROR("spi_master_open");
    return err;
  }

  if (spi_leds->options.gpio_out) {
    gpio_out_set(spi_leds->options.gpio_out, spi_leds->options.gpio_out_pins);
  }

  while (len) {
    if ((ret = spi_master_write(spi_leds->spi_master, buf, len)) < 0) {
      LOG_ERROR("spi_master_write");
      goto error;
    }

    LOG_DEBUG("spi_leds=%p write %p @ %u -> %d", spi_leds, buf, len, ret);

    buf += ret;
    len -= ret;
  }

  // wait for write TX to complete before clearing gpio to release bus
  if ((ret = spi_master_flush(spi_leds->spi_master)) < 0) {
    LOG_ERROR("spi_master_flush");
    goto error;
  }

error:
  if (spi_leds->options.gpio_out) {
    gpio_out_clear(spi_leds->options.gpio_out);
  }

  if ((err = spi_master_close(spi_leds->spi_master))) {
    LOG_ERROR("spi_master_close");
    return err;
  }

  return ret;
}
