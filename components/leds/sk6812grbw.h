#pragma once

#include <leds.h>

// 32 bits per pixel, 2 bits per I2S byte
#define SK6812_GRBW_I2S_SIZE (4 * 4)

union sk6812grbw_pixel {
  struct {
    uint8_t w, b, r, g;
  };

  // aligned with 0xGGRRBBWW on little-endian architectures
  uint32_t grbw;
};

struct leds_protocol_sk6812grbw {
  union sk6812grbw_pixel *pixels;
};

int leds_init_sk6812grbw(struct leds_protocol_sk6812grbw *protocol, const struct leds_options *options);
int leds_tx_sk6812grbw(struct leds_protocol_sk6812grbw *protocol, const struct leds_options *options);

void sk6812grbw_set_frame(struct leds_protocol_sk6812grbw *protocol, unsigned index, struct spi_led_color color);
void sk6812grbw_set_frames(struct leds_protocol_sk6812grbw *protocol, unsigned count, struct spi_led_color color);

unsigned sk6812grbw_count_active(struct leds_protocol_sk6812grbw *protocol, unsigned count);

#if CONFIG_LEDS_UART_ENABLED
  /* interfaces/uart/sk6812grbw.c */
  int leds_tx_uart_sk6812grbw(const struct leds_options *options, union sk6812grbw_pixel *pixels, unsigned count);
#endif

#if CONFIG_LEDS_I2S_ENABLED
  /* interfaces/i2s/sk6812grbw.c */
  int leds_tx_i2s_sk6812grbw(const struct leds_options *options, union sk6812grbw_pixel *pixels, unsigned count);
#endif
