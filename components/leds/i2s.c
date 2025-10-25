#include <leds.h>
#include "leds.h"

#include <logging.h>

#if CONFIG_LEDS_I2S_ENABLED
  i2s_port_t leds_interface_i2s_port(enum leds_interface interface)
  {
    switch(interface) {
    #if LEDS_I2S_INTERFACE_COUNT > 0
      case LEDS_INTERFACE_I2S0:
        return I2S_PORT_0;
    #endif

    #if LEDS_I2S_INTERFACE_COUNT > 1
      case LEDS_INTERFACE_I2S1:
        return I2S_PORT_1;
    #endif

      default:
        LOG_FATAL("%u", interface);
    }
  }

  size_t leds_i2s_serial_buffer_size(enum leds_protocol protocol, unsigned led_count)
  {
    const struct leds_protocol_type *protocol_type = leds_protocol_type(protocol);

    if (protocol_type->i2s_interface_mode) {
      return leds_interface_i2s_buffer_size(protocol_type->i2s_interface_mode, led_count, 0);
    } else {
      // not defined
      return 0;
    }
  }

  size_t leds_i2s_serial_buffer_align(enum leds_protocol protocol)
  {
    const struct leds_protocol_type *protocol_type = leds_protocol_type(protocol);

    if (protocol_type->i2s_interface_mode) {
      return leds_interface_i2s_buffer_align(protocol_type->i2s_interface_mode, 0);
    } else {
      // not defined
      return 0;
    }
  }
#endif

#if CONFIG_LEDS_I2S_ENABLED && I2S_OUT_PARALLEL_SUPPORTED
  size_t leds_i2s_parallel_buffer_size(enum leds_protocol protocol, unsigned led_count, unsigned parallel)
  {
    const struct leds_protocol_type *protocol_type = leds_protocol_type(protocol);

    if (protocol_type->i2s_interface_mode) {
      return leds_interface_i2s_buffer_size(protocol_type->i2s_interface_mode, led_count, parallel);
    } else {
      // not defined
      return 0;
    }
  }

  size_t leds_i2s_parallel_buffer_align(enum leds_protocol protocol, unsigned parallel)
  {
    const struct leds_protocol_type *protocol_type = leds_protocol_type(protocol);

    if (protocol_type->i2s_interface_mode) {
      return leds_interface_i2s_buffer_align(protocol_type->i2s_interface_mode, parallel);
    } else {
      // not defined
      return 0;
    }
  }
#endif
