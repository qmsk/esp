#pragma once

#include <leds.h>

#include "../limit.h"

#if CONFIG_LEDS_I2S_ENABLED

enum leds_interface_i2s_mode {
  LEDS_INTERFACE_I2S_MODE_NONE = 0,

  LEDS_INTERFACE_I2S_MODE_32BIT_BCK,          // 32-bit with bit-clock, 32x0-bit start frame + 32x0-bit end frame with at least one bit per pixel

  LEDS_INTERFACE_I2S_MODE_24BIT_1U200_4X4_80UL, // 24 bits @ 1.200us/bit, 4-bit symbols * 4-bit LUT, 80us low reset -> uint16_t[6]
  LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL, // 24 bits @ 1.250us/bit, 4-bit symbols * 4-bit LUT, 80us low reset -> uint16_t[6]
  LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL, // 32 bits @ 1.250us/bit, 4-bit symbols * 4-bit LUT, 80us low reset -> uint16_t[8]
};

#define LEDS_INTERFACE_I2S_MODE_32BIT_BCK_START_FRAME 0x00000000

static inline uint32_t leds_interface_i2s_mode_start_frame(enum leds_interface_i2s_mode mode)
{
  switch (mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
      return LEDS_INTERFACE_I2S_MODE_32BIT_BCK_START_FRAME;

    default:
      abort();
  }
}

union leds_interface_i2s_buf {
  uint32_t i2s_mode_32bit[1];
  uint16_t i2s_mode_24bit_4x4[6];
  uint16_t i2s_mode_32bit_4x4[8];

  uint32_t i2s_mode_32bit_parallel8[8][1];
  uint16_t i2s_mode_24bit_4x4_parallel8[8][6];
  uint16_t i2s_mode_32bit_4x4_parallel8[8][8];
};

/* Size of single pixel buffer */
size_t leds_interface_i2s_buf_size(enum leds_interface_i2s_mode mode, unsigned parallel);

union leds_interface_i2s_func {
  void (*i2s_mode_32bit)(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
  void (*i2s_mode_24bit_4x4)(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
  void (*i2s_mode_32bit_4x4)(uint16_t buf[8], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
};

#define LEDS_INTERFACE_I2S_FUNC(type, func) ((union leds_interface_i2s_func) { .type = func })

struct leds_interface_i2s {
  bool setup; // persistent setup()

  enum leds_interface_i2s_mode mode;
  union leds_interface_i2s_func func;
  union leds_interface_i2s_buf *buf;

  unsigned parallel;
  unsigned repeat;

  struct i2s_out *i2s_out;
  struct i2s_out_options i2s_out_options;
  
  struct leds_interface_options_gpio gpio;
  struct leds_interface_i2s_stats *stats;
};

size_t leds_interface_i2s_buffer_size(enum leds_interface_i2s_mode mode, unsigned led_count, unsigned pin_count);
size_t leds_interface_i2s_buffer_align(enum leds_interface_i2s_mode mode, unsigned pin_count);

int leds_interface_i2s_init(struct leds_interface_i2s *interface, const struct leds_interface_i2s_options *options, enum leds_interface_i2s_mode mode, union leds_interface_i2s_func func, unsigned count, struct leds_interface_i2s_stats *stats);
int leds_interface_i2s_setup(struct leds_interface_i2s *interface);
int leds_interface_i2s_tx(struct leds_interface_i2s *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit);
int leds_interface_i2s_close(struct leds_interface_i2s *interface);

#endif
