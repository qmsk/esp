#pragma once

#include <leds.h>

// 24 bits per pixel, 2 bits per I2S byte
#define WS2811_I2S_SIZE (3 * 4)

union ws2811_pixel {
  struct {
    uint8_t b, g, r;
  };

  // aligned with 0xXXRRGGBB on little-endian architectures
  uint32_t _rgb;
};

struct leds_protocol_ws2811 {
  union ws2811_pixel *pixels;
};

int leds_init_ws2811(struct leds_protocol_ws2811 *protocol, const struct leds_options *options);
int leds_tx_ws2811(struct leds_protocol_ws2811 *protocol, const struct leds_options *options);


void ws2811_set_frame(struct leds_protocol_ws2811 *protocol, unsigned index, struct spi_led_color color);
void ws2811_set_frames(struct leds_protocol_ws2811 *protocol, unsigned count, struct spi_led_color color);

unsigned ws2811_count_active(struct leds_protocol_ws2811 *protocol, unsigned count);

/* ws2811_uart.c */
int leds_tx_uart_ws2811(const struct leds_options *options, union ws2811_pixel *pixels, unsigned count);

/* ws2811_i2s.c */
int leds_tx_i2s_ws2811(const struct leds_options *options, union ws2811_pixel *pixels, unsigned count);
