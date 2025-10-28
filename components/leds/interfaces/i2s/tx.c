#include "../i2s.h"
#include "../../leds.h"
#include "../../stats.h"

#include <logging.h>

static int leds_interface_i2s_tx_32bit_bck_serial32(struct leds_interface_i2s *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
{
  int err;

  // start frame
  uint32_t start_frame = leds_interface_i2s_mode_start_frame(interface->mode);

  if ((err = i2s_out_write_serial32(interface->i2s_out, &start_frame, 1, interface->timeout))) {
    LOG_ERROR("i2s_out_write_serial32");
    return err;
  }

  // pixel frames
  for (unsigned i = 0; i < count; i++) {
    // 32-bit pixel data
    interface->func.i2s_mode_32bit(interface->buf->i2s_mode_32bit, pixels, i, limit);

    if ((err = i2s_out_write_serial32(interface->i2s_out, interface->buf->i2s_mode_32bit, 1, interface->timeout))) {
      LOG_ERROR("i2s_out_write_serial32");
      return err;
    }
  }

  return 0;
}

static int leds_interface_i2s_tx_24bit_4x4_serial16(struct leds_interface_i2s *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
{
  int err;

  for (unsigned i = 0; i < count; i++) {
    // 6x16-bit pixel data
    interface->func.i2s_mode_24bit_4x4(interface->buf->i2s_mode_24bit_4x4, pixels, i, limit);

    if ((err = i2s_out_write_serial16(interface->i2s_out, interface->buf->i2s_mode_24bit_4x4, 6, interface->timeout))) {
      LOG_ERROR("i2s_out_write_serial16");
      return err;
    }
  }

  return 0;
}

static int leds_interface_i2s_tx_32bit_4x4_serial16(struct leds_interface_i2s *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
{
  int err;

  for (unsigned i = 0; i < count; i++) {
    // 8x16-bit pixel data
    interface->func.i2s_mode_32bit_4x4(interface->buf->i2s_mode_32bit_4x4, pixels, i, limit);

    if ((err = i2s_out_write_serial16(interface->i2s_out, interface->buf->i2s_mode_32bit_4x4, 8, interface->timeout))) {
      LOG_ERROR("i2s_out_write_serial16");
      return err;
    }
  }

  return 0;
}

#if I2S_OUT_PARALLEL_SUPPORTED
  static int leds_interface_i2s_tx_32bit_bck_parallel8(struct leds_interface_i2s *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
  {
    unsigned length = count / interface->parallel;
    int err;

    // start frame
    uint32_t start_frame[8] = { [0 ... 7] = leds_interface_i2s_mode_start_frame(interface->mode) };

    if ((err = i2s_out_write_parallel8x32(interface->i2s_out, start_frame, 1, interface->timeout))) {
      LOG_ERROR("i2s_out_write_parallel8x32");
      return err;
    }

    for (unsigned i = 0; i < length; i++) {
      // 8 sets of 32-bit pixel data
      for (unsigned j = 0; j < interface->parallel && j < 8; j++) {
        interface->func.i2s_mode_32bit(interface->buf->i2s_mode_32bit_parallel8[j], pixels, j * length + i, limit);
      }

      if ((err = i2s_out_write_parallel8x32(interface->i2s_out, (uint32_t *) interface->buf->i2s_mode_32bit_parallel8, 1, interface->timeout))) {
        LOG_ERROR("i2s_out_write_parallel8x32");
        return err;
      }
    }

    return 0;
  }

  static int leds_interface_i2s_tx_24bit_4x4_parallel8(struct leds_interface_i2s *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
  {
    unsigned length = count / interface->parallel;
    int err;

    for (unsigned i = 0; i < length; i++) {
      // 8 sets of 6x16-bit pixel data
      for (unsigned j = 0; j < interface->parallel && j < 8; j++) {
        interface->func.i2s_mode_24bit_4x4(interface->buf->i2s_mode_24bit_4x4_parallel8[j], pixels, j * length + i, limit);
      }

      if ((err = i2s_out_write_parallel8x16(interface->i2s_out, (uint16_t *) interface->buf->i2s_mode_24bit_4x4_parallel8, 6, interface->timeout))) {
        LOG_ERROR("i2s_out_write_parallel8x16");
        return err;
      }
    }

    return 0;
  }

  static int leds_interface_i2s_tx_32bit_4x4_parallel8(struct leds_interface_i2s *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
  {
    unsigned length = count / interface->parallel;
    int err;

    for (unsigned i = 0; i < length; i++) {
      // 8 sets of 8x16-bit pixel data
      for (unsigned j = 0; j < interface->parallel && j < 8; j++) {
        interface->func.i2s_mode_32bit_4x4(interface->buf->i2s_mode_32bit_4x4_parallel8[j], pixels, j * length + i, limit);
      }

      if ((err = i2s_out_write_parallel8x16(interface->i2s_out, (uint16_t *) interface->buf->i2s_mode_32bit_4x4_parallel8, 8, interface->timeout))) {
        LOG_ERROR("i2s_out_write_parallel8x16");
        return err;
      }
    }

    return 0;
  }
#endif

static int leds_interface_i2s_tx_write(struct leds_interface_i2s *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
{
  switch(interface->mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
    #if I2S_OUT_PARALLEL_SUPPORTED
      if (interface->parallel) {
        return leds_interface_i2s_tx_32bit_bck_parallel8(interface, pixels, count, limit);
      } else {
        return leds_interface_i2s_tx_32bit_bck_serial32(interface, pixels, count, limit);
      }
    #else
      return leds_interface_i2s_tx_32bit_bck_serial32(interface, pixels, count, limit);
    #endif

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U200_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
    #if I2S_OUT_PARALLEL_SUPPORTED
      if (interface->parallel) {
        return leds_interface_i2s_tx_24bit_4x4_parallel8(interface, pixels, count, limit);
      } else {
        return leds_interface_i2s_tx_24bit_4x4_serial16(interface, pixels, count, limit);
      }
    #else
      return leds_interface_i2s_tx_24bit_4x4_serial16(interface, pixels, count, limit);
    #endif

    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
    #if I2S_OUT_PARALLEL_SUPPORTED
      if (interface->parallel) {
        return leds_interface_i2s_tx_32bit_4x4_parallel8(interface, pixels, count, limit);
      } else {
        return leds_interface_i2s_tx_32bit_4x4_serial16(interface, pixels, count, limit);
      }
    #else
      return leds_interface_i2s_tx_32bit_4x4_serial16(interface, pixels, count, limit);
    #endif

    default:
      LOG_FATAL("unknown mode=%08x", interface->mode);
  }
}


int leds_interface_i2s_tx(struct leds_interface_i2s *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
{
  bool setup = !interface->setup; // sync setup() -> write -> flush -> close()?
  int err = 0;

  if (setup) {
    if ((err = leds_interface_i2s_setup(interface))) {
      LOG_ERROR("leds_interface_i2s_setup");
      return err;
    }
  }

  WITH_STATS_TIMER(&interface->stats->write) {
    if ((err = leds_interface_i2s_tx_write(interface, pixels, count, limit))) {
      goto error;
    }
  }

  if (interface->repeat) {
    if ((err = i2s_out_repeat(interface->i2s_out, interface->repeat))) {
      LOG_ERROR("i2s_out_repeat");
      goto error;
    }
  }

  if (setup) {
    // sync, wait for done before close
    WITH_STATS_TIMER(&interface->stats->flush) {
      if ((err = i2s_out_flush(interface->i2s_out, interface->timeout))) {
        LOG_ERROR("i2s_out_flush");
        goto error;
      }
    }
  } else {
    // async, do not wait for done
    WITH_STATS_TIMER(&interface->stats->start) {
      if ((err = i2s_out_start(interface->i2s_out, interface->timeout))) {
        LOG_ERROR("i2s_out_start");
        goto error;
      }
    }
  }

error:
  if (setup) {
    if (leds_interface_i2s_close(interface)) {
      LOG_WARN("leds_interface_i2s_close");
    }
  }

  return err;
}
