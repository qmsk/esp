#pragma once

#include <spi_leds.h>

union ws2812b_pixel {
  struct {
    uint8_t b, r, g;
  };

  // aligned with 0xXXGGRRBB on little-endian architectures
  uint32_t _grb;
};

struct spi_leds_protocol_ws2812b {
  union ws2812b_pixel *pixels;
};

int spi_leds_init_ws2812b(struct spi_leds_protocol_ws2812b *protocol, const struct spi_leds_options *options);
int spi_leds_tx_ws2812b(struct spi_leds_protocol_ws2812b *protocol, const struct spi_leds_options *options);


void ws2812b_set_frame(struct spi_leds_protocol_ws2812b *protocol, unsigned index, struct spi_led_color color);
void ws2812b_set_frames(struct spi_leds_protocol_ws2812b *protocol, unsigned count, struct spi_led_color color);

unsigned ws2812b_count_active(struct spi_leds_protocol_ws2812b *protocol, unsigned count);

/* ws2812b_uart.c */
int spi_leds_tx_uart_ws2812b(const struct spi_leds_options *options, union ws2812b_pixel *pixels, unsigned count);
