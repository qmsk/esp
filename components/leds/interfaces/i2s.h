#pragma once

#include "../interface.h"

#if CONFIG_LEDS_I2S_ENABLED

enum leds_interface_i2s_mode {
  LEDS_INTERFACE_I2S_MODE_32BIT_BCK,          // 32-bit with bit-clock, 32-bit start frame + end frame bit per pixel

  LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL, // 24 bits @ 1.250us/bit, 4-bit symbols * 4-bit LUT, 80us low reset -> uint16_t[6]
  LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL, // 32 bits @ 1.250us/bit, 4-bit symbols * 4-bit LUT, 80us low reset -> uint16_t[8]
};

struct leds_interface_i2s_tx {
  void *data;
  unsigned count;
  const struct leds_limit *limit;

  union {
    uint32_t i2s_mode_32bit;
  } start_frame;

  union {
    void (*i2s_mode_32bit)(uint32_t buf[1], void *data, unsigned index, const struct leds_limit *limit);
    void (*i2s_mode_24bit_4x4)(uint16_t buf[6], void *data, unsigned index, const struct leds_limit *limit);
    void (*i2s_mode_32bit_4x4)(uint16_t buf[8], void *data, unsigned index, const struct leds_limit *limit);
  } func;
};

size_t leds_interface_i2s_buffer_size(enum leds_interface_i2s_mode mode, unsigned led_count, unsigned pin_count);
size_t leds_interface_i2s_buffer_align(enum leds_interface_i2s_mode mode, unsigned pin_count);

int leds_interface_i2s_tx(const struct leds_interface_i2s_options *options, enum leds_interface_i2s_mode mode, struct leds_interface_i2s_tx tx);

#endif
