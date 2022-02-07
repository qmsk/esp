#pragma once

#include <leds.h>

// 24 bits per pixel, 2 bits per I2S byte
#define WS2812B_I2S_SIZE (3 * 4)

union ws2812b_pixel {
  struct {
    uint8_t b, r, g;
  };

  // aligned with 0xXXGGRRBB on little-endian architectures
  uint32_t _grb;
};

struct leds_protocol_ws2812b {
  union ws2812b_pixel *pixels;
};

int leds_init_ws2812b(struct leds_protocol_ws2812b *protocol, const struct leds_options *options);
int leds_tx_ws2812b(struct leds_protocol_ws2812b *protocol, const struct leds_options *options);


void ws2812b_set_frame(struct leds_protocol_ws2812b *protocol, unsigned index, struct spi_led_color color);
void ws2812b_set_frames(struct leds_protocol_ws2812b *protocol, unsigned count, struct spi_led_color color);

unsigned ws2812b_count_active(struct leds_protocol_ws2812b *protocol, unsigned count);

/* ws2812b_uart.c */
int leds_tx_uart_ws2812b(const struct leds_options *options, union ws2812b_pixel *pixels, unsigned count);

/* ws2812b_i2s.c */
int leds_tx_i2s_ws2812b(const struct leds_options *options, union ws2812b_pixel *pixels, unsigned count);
