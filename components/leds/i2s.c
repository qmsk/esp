#include <leds.h>
#include "leds.h"

#include <logging.h>

#if CONFIG_LEDS_I2S_ENABLED

  size_t leds_i2s_serial_buffer_size(enum leds_protocol protocol, unsigned led_count)
  {
    switch (protocol) {
      case LEDS_PROTOCOL_APA102:
      case LEDS_PROTOCOL_P9813:
        // not defined
        return 0;

      case LEDS_PROTOCOL_WS2812B:
        return leds_interface_i2s_buffer_size(LEDS_PROTOCOL_WS2812B_INTERFACE_I2S_MODE, led_count, 0);
      case LEDS_PROTOCOL_SK6812_GRBW:
        return leds_interface_i2s_buffer_size(LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE, led_count, 0);
      case LEDS_PROTOCOL_WS2811:
        return leds_interface_i2s_buffer_size(LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE, led_count, 0);
      case LEDS_PROTOCOL_SK9822:
        return leds_interface_i2s_buffer_size(LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE, led_count, 0);

      default:
        // unknown
        LOG_FATAL("invalid protocol=%d", protocol);
    }
  }

  size_t leds_i2s_serial_buffer_align(enum leds_protocol protocol)
  {
    switch (protocol) {
      case LEDS_PROTOCOL_APA102:
      case LEDS_PROTOCOL_P9813:
        // not defined
        return 0;

      case LEDS_PROTOCOL_WS2812B:
        return leds_interface_i2s_buffer_align(LEDS_PROTOCOL_WS2812B_INTERFACE_I2S_MODE, 0);
      case LEDS_PROTOCOL_SK6812_GRBW:
        return leds_interface_i2s_buffer_align(LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE, 0);
      case LEDS_PROTOCOL_WS2811:
        return leds_interface_i2s_buffer_align(LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE, 0);
      case LEDS_PROTOCOL_SK9822:
        return leds_interface_i2s_buffer_align(LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE, 0);

      default:
        // unknown
        LOG_FATAL("invalid protocol=%d", protocol);
    }
  }

  size_t leds_i2s_parallel_buffer_size(enum leds_protocol protocol, unsigned led_count, unsigned pin_count)
  {
    switch (protocol) {
      case LEDS_PROTOCOL_APA102:
      case LEDS_PROTOCOL_P9813:
        // not defined
        return 0;

      case LEDS_PROTOCOL_WS2812B:
        return leds_interface_i2s_buffer_size(LEDS_PROTOCOL_WS2812B_INTERFACE_I2S_MODE, led_count, pin_count);
      case LEDS_PROTOCOL_SK6812_GRBW:
        return leds_interface_i2s_buffer_size(LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE, led_count, pin_count);
      case LEDS_PROTOCOL_WS2811:
        return leds_interface_i2s_buffer_size(LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE, led_count, pin_count);
      case LEDS_PROTOCOL_SK9822:
        return leds_interface_i2s_buffer_size(LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE, led_count, pin_count);

      default:
        // unknown
        LOG_FATAL("invalid protocol=%d", protocol);
    }
  }

  size_t leds_i2s_parallel_buffer_align(enum leds_protocol protocol, unsigned pin_count)
  {
    switch (protocol) {
      case LEDS_PROTOCOL_APA102:
      case LEDS_PROTOCOL_P9813:
        // not defined
        return 0;

      case LEDS_PROTOCOL_WS2812B:
        return leds_interface_i2s_buffer_align(LEDS_PROTOCOL_WS2812B_INTERFACE_I2S_MODE, pin_count);
      case LEDS_PROTOCOL_SK6812_GRBW:
        return leds_interface_i2s_buffer_align(LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE, pin_count);
      case LEDS_PROTOCOL_WS2811:
        return leds_interface_i2s_buffer_align(LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE, pin_count);
      case LEDS_PROTOCOL_SK9822:
        return leds_interface_i2s_buffer_align(LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE, pin_count);

      default:
        // unknown
        LOG_FATAL("invalid protocol=%d", protocol);
    }
  }

#endif
