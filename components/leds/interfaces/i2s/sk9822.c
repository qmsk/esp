#include "../sk9822.h"
#include "../../leds.h"
#include "../../stats.h"

#include <logging.h>


int leds_tx_i2s_sk9822(const struct leds_interface_i2s_options *options, union sk9822_pixel *pixels, unsigned count, struct leds_limit limit)
{
  struct leds_interface_i2s_stats *stats = &leds_interface_stats.i2s;
  struct i2s_out_options i2s_out_options = {
    .clock        = i2s_out_clock(options->clock_rate),

    .eof_value    = SK9822_END_FRAME_UINT32,
    .eof_count    = SK9822_END_FRAME_COUNT(count),

    // shared IO pins
    .pin_mutex    = options->pin_mutex,
    .pin_timeout  = options->pin_timeout,
#if LEDS_I2S_GPIO_PINS_ENABLED
    .bck_gpio       = options->clock_pin,
    .data_gpio      = options->data_pin,
    .inv_data_gpio  = options->inv_data_pin,
#endif
  };
  int err;

  WITH_STATS_TIMER(&stats->open) {
    if ((err = i2s_out_open(options->i2s_out, i2s_out_options))) {
      LOG_ERROR("i2s_out_open");
      return err;
    }
  }

#if CONFIG_LEDS_GPIO_ENABLED
  if (options->gpio.gpio_out) {
    gpio_out_set(options->gpio.gpio_out, options->gpio.gpio_out_pins);
  }
#endif

  WITH_STATS_TIMER(&stats->tx) {
    // start frame
    uint32_t start_frame = SK9822_START_FRAME_UINT32;

    if ((err = i2s_out_write_serial32(options->i2s_out, &start_frame, 1))) {
      LOG_ERROR("i2s_out_write_all");
      goto error;
    }

    // pixel frames
    for (unsigned i = 0; i < count; i++) {
      // transmit in 32-bit little-endian order
      uint32_t xbgr = sk9822_pixel_limit(pixels[i], limit).xbgr;

      if ((err = i2s_out_write_serial32(options->i2s_out, &xbgr, 1))) {
        LOG_ERROR("i2s_out_write_all");
        goto error;
      }
    }

    if ((err = i2s_out_flush(options->i2s_out))) {
      LOG_ERROR("i2s_out_flush");
      goto error;
    }
  }

error:
#if CONFIG_LEDS_GPIO_ENABLED
  if (options->gpio.gpio_out) {
    gpio_out_clear(options->gpio.gpio_out);
  }
#endif

  if ((err = i2s_out_close(options->i2s_out))) {
    LOG_ERROR("i2s_out_close");
    return err;
  }

  return err;
}
