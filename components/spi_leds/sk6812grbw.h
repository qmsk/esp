#pragma once

#include <spi_leds.h>

// 32 bits per pixel, 2 bits per I2S byte
#define SK6812_GRBW_I2S_SIZE (4 * 4)

union sk6812grbw_pixel {
  struct {
    uint8_t w, b, r, g;
  };

  // aligned with 0xGGRRBBWW on little-endian architectures
  uint32_t grbw;
};

struct spi_leds_protocol_sk6812grbw {
  union sk6812grbw_pixel *pixels;
};

int spi_leds_init_sk6812grbw(struct spi_leds_protocol_sk6812grbw *protocol, const struct spi_leds_options *options);
int spi_leds_tx_sk6812grbw(struct spi_leds_protocol_sk6812grbw *protocol, const struct spi_leds_options *options);

void sk6812grbw_set_frame(struct spi_leds_protocol_sk6812grbw *protocol, unsigned index, struct spi_led_color color);
void sk6812grbw_set_frames(struct spi_leds_protocol_sk6812grbw *protocol, unsigned count, struct spi_led_color color);

unsigned sk6812grbw_count_active(struct spi_leds_protocol_sk6812grbw *protocol, unsigned count);

/* sk6812grbw_uart.c */
int spi_leds_tx_uart_sk6812grbw(const struct spi_leds_options *options, union sk6812grbw_pixel *pixels, unsigned count);

/* sk6812grbw_i2s.c */
int spi_leds_tx_i2s_sk6812grbw(const struct spi_leds_options *options, union sk6812grbw_pixel *pixels, unsigned count);
