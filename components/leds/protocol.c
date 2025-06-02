#include "protocol.h"
#include "protocols/apa102.h"
#include "protocols/p9813.h"
#include "protocols/sk6812grbw.h"
#include "protocols/ws2811.h"
#include "protocols/ws2812b.h"
#include "protocols/sk9822.h"
#include "protocols/sm16703.h"

#include <logging.h>

const struct leds_protocol_type *leds_protocol_types[LEDS_PROTOCOLS_COUNT] = {
  [LEDS_PROTOCOL_APA102]      = &leds_protocol_apa102,
  [LEDS_PROTOCOL_P9813]       = &leds_protocol_p9813,

  [LEDS_PROTOCOL_WS2812B_GRB] = &leds_protocol_ws2812b_grb,
  [LEDS_PROTOCOL_WS2812B_RGB] = &leds_protocol_ws2812b_rgb,
  [LEDS_PROTOCOL_SK6812_GRBW] = &leds_protocol_sk6812grbw,
  [LEDS_PROTOCOL_WS2811_RGB]  = &leds_protocol_ws2811_rgb,
  [LEDS_PROTOCOL_WS2811_GRB]  = &leds_protocol_ws2811_grb,
  [LEDS_PROTOCOL_SK9822]      = &leds_protocol_sk9822,
  [LEDS_PROTOCOL_SM16703]     = &leds_protocol_sm16703,
};

const struct leds_protocol_type *leds_protocol_type(enum leds_protocol protocol)
{
  if (protocol < LEDS_PROTOCOLS_COUNT && leds_protocol_types[protocol]) {
    return leds_protocol_types[protocol];
  } else {
    LOG_FATAL("invalid protocol=%d", protocol);
  }
}

enum leds_interface leds_interface_for_protocol(enum leds_protocol protocol)
{
  const struct leds_protocol_type *protocol_type = leds_protocol_type(protocol);

#if CONFIG_LEDS_I2S_ENABLED
  if (protocol_type->i2s_interface_mode) {
    return LEDS_INTERFACE_I2S;
  }
#endif

#if CONFIG_LEDS_UART_ENABLED
  if (protocol_type->uart_interface_mode) {
    return LEDS_INTERFACE_UART;
  }
#endif

#if CONFIG_LEDS_SPI_ENABLED
  if (protocol_type->spi_interface_mode) {
    return LEDS_INTERFACE_SPI;
  }
#endif

  return LEDS_INTERFACE_NONE;
}

enum leds_parameter_type leds_parameter_type_for_protocol(enum leds_protocol protocol)
{
  const struct leds_protocol_type *protocol_type = leds_protocol_type(protocol);

  return protocol_type->parameter_type;
}

uint8_t leds_parameter_default_for_protocol(enum leds_protocol protocol)
{
  return leds_parameter_default_for_type(leds_parameter_type_for_protocol(protocol));
}

enum leds_power_mode leds_power_mode_for_protocol(enum leds_protocol protocol)
{
  const struct leds_protocol_type *protocol_type = leds_protocol_type(protocol);

  return protocol_type->power_mode;
}
