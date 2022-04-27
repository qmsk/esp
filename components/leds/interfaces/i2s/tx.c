#include "../i2s.h"
#include "../../leds.h"
#include "../../stats.h"

#include <logging.h>

static int leds_interface_i2s_tx_32bit_bck(struct i2s_out *i2s_out, struct leds_interface_i2s_tx tx)
{
  int err;

  // start frame
  uint32_t start_frame = tx.start_frame.i2s_mode_32bit;

  if ((err = i2s_out_write_serial32(i2s_out, &start_frame, 1))) {
    LOG_ERROR("i2s_out_write_serial32");
    return err;
  }

  // pixel frames
  for (unsigned i = 0; i < tx.count; i++) {
    // transmit in 32-bit little-endian order
    uint32_t buf[1];

    tx.pixel_func.i2s_mode_32bit(buf, tx.protocol, i, tx.limit);

    if ((err = i2s_out_write_serial32(i2s_out, buf, 1))) {
      LOG_ERROR("i2s_out_write_serial32");
      return err;
    }
  }

  return 0;
}

static int leds_interface_i2s_tx_24bit_4x4_serial16(struct i2s_out *i2s_out, struct leds_interface_i2s_tx tx)
{
  int err;

  for (unsigned i = 0; i < tx.count; i++) {
    uint16_t buf[6];

    tx.pixel_func.i2s_mode_24bit_4x4(buf, tx.protocol, i, tx.limit);

    if ((err = i2s_out_write_serial16(i2s_out, buf, 6))) {
      LOG_ERROR("i2s_out_write_serial16");
      return err;
    }
  }

  return 0;
}

static int leds_interface_i2s_tx_32bit_4x4_serial16(struct i2s_out *i2s_out, struct leds_interface_i2s_tx tx)
{
  int err;

  for (unsigned i = 0; i < tx.count; i++) {
    uint16_t buf[8];

    tx.pixel_func.i2s_mode_32bit_4x4(buf, tx.protocol, i, tx.limit);

    if ((err = i2s_out_write_serial16(i2s_out, buf, 8))) {
      LOG_ERROR("i2s_out_write_serial16");
      return err;
    }
  }

  return 0;
}

static int leds_interface_i2s_tx_24bit_4x4_parallel8(struct i2s_out *i2s_out, struct leds_interface_i2s_tx tx, unsigned parallel)
{
  int err;

  for (unsigned i = 0; i < tx.count / parallel; i++) {
    // 8 sets of 6x16-bit pixel data
    uint16_t buf[8][6] = {};

    for (unsigned j = 0; j < parallel && j < 8; j++) {
      tx.pixel_func.i2s_mode_24bit_4x4(buf[j], tx.protocol, i, tx.limit);
    }

    if ((err = i2s_out_write_parallel8x16(i2s_out, buf, 6))) {
      LOG_ERROR("i2s_out_write_parallel8x16");
      return err;
    }
  }

  return 0;
}

static int leds_interface_i2s_tx_32bit_4x4_parallel8(struct i2s_out *i2s_out, struct leds_interface_i2s_tx tx, unsigned parallel)
{
  int err;

  for (unsigned i = 0; i < tx.count / parallel; i++) {
    // 8 sets of 6x16-bit pixel data
    uint16_t buf[8][8] = {};

    for (unsigned j = 0; j < parallel && j < 8; j++) {
      tx.pixel_func.i2s_mode_32bit_4x4(buf[j], tx.protocol, i, tx.limit);
    }

    if ((err = i2s_out_write_parallel8x16(i2s_out, buf, 8))) {
      LOG_ERROR("i2s_out_write_parallel8x16");
      return err;
    }
  }

  return 0;
}

static int leds_interface_i2s_tx_mode(struct i2s_out *i2s_out, enum leds_interface_i2s_mode mode, struct leds_interface_i2s_tx tx, unsigned parallel)
{
  switch(mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
      return leds_interface_i2s_tx_32bit_bck(i2s_out, tx);

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
      if (parallel) {
        return leds_interface_i2s_tx_24bit_4x4_parallel8(i2s_out, tx, parallel);
      } else {
        return leds_interface_i2s_tx_24bit_4x4_serial16(i2s_out, tx);
      }

    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
      if (parallel) {
        return leds_interface_i2s_tx_32bit_4x4_parallel8(i2s_out, tx, parallel);
      } else {
        return leds_interface_i2s_tx_32bit_4x4_serial16(i2s_out, tx);
      }

    default:
      LOG_ERROR("unknown mode=%08x", mode);
      return -1;
  }
}

int leds_interface_i2s_tx(const struct leds_interface_i2s_options *options, enum leds_interface_i2s_mode mode, struct leds_interface_i2s_tx tx)
{
  struct leds_interface_i2s_stats *stats = &leds_interface_stats.i2s;
  struct i2s_out_options i2s_out_options = {
    // shared IO pins
    .pin_mutex    = options->pin_mutex,
    .pin_timeout  = options->pin_timeout,
#if LEDS_I2S_GPIO_PINS_ENABLED
    .bck_gpio       = GPIO_NUM_NC,
    .data_gpio      = options->data_pin,
    .inv_data_gpio  = options->inv_data_pin,
#endif
  };
  int err;

  switch(mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
      // TODO: parallel?
      i2s_out_options.mode = I2S_OUT_MODE_32BIT_SERIAL;

      break;

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
      if (options->data_pins_count) {
        i2s_out_options.mode = I2S_OUT_MODE_8BIT_PARALLEL;
      } else {
        // using 4x4bit -> 16-bit samples
        i2s_out_options.mode = I2S_OUT_MODE_16BIT_SERIAL;
      }

      break;
  }

  switch(mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
      i2s_out_options.clock = i2s_out_clock(options->clock_rate);

      break;

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
      // 3.2MHz bit clock => 0.3125us per I2S bit
      // four I2S bits per 1.25us protocol bit
      i2s_out_options.clock = I2S_OUT_CLOCK_3M2;

      break;
  }

  switch(mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
      // XXX: required to workaround I2S start glitch looping previous data bits
      i2s_out_options.eof_value = 0x00000000;

      // one clock cycle per pixel, min 32 cycles
      i2s_out_options.eof_count = (1 + tx.count / 32);

      break;

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
      // 1.25us per 4-bit = 2.5us per byte * four bytes per I2S sample = 10us per 32-bit I2S sample
      i2s_out_options.eof_value = 0x00000000;

      // hold low for 8 * 10us
      i2s_out_options.eof_count = 8;

      break;
  }


  switch(mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
      i2s_out_options.bck_gpio = options->clock_pin;

      break;

    default:
      i2s_out_options.bck_gpio = GPIO_NUM_NC;

      break;
  }

  if (options->data_pins_count) {
    for (int i = 0; i < I2S_OUT_PARALLEL_SIZE; i++) {
      i2s_out_options.data_gpios[i] = (i < options->data_pins_count && i < LEDS_I2S_DATA_PINS_SIZE) ? options->data_pins[i] : GPIO_NUM_NC;
      i2s_out_options.inv_data_gpios[i] = (i < options->data_pins_count && i < LEDS_I2S_DATA_PINS_SIZE) ? options->inv_data_pins[i] : GPIO_NUM_NC;
    }
  } else {
    i2s_out_options.data_gpio = options->data_pin;
    i2s_out_options.inv_data_gpio = options->inv_data_pin;
  }

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
    if ((err = leds_interface_i2s_tx_mode(options->i2s_out, mode, tx, options->data_pins_count))) {
      goto error;
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
