#include "protocol.h"
#include "protocols/apa102.h"
#include "protocols/p9813.h"
#include "protocols/sk6812grbw.h"
#include "protocols/ws2811.h"
#include "protocols/ws2812b.h"
#include "protocols/sk9822.h"

#include <logging.h>

const struct leds_protocol_type *leds_protocol_types[LEDS_PROTOCOLS_COUNT] = {
  [LEDS_PROTOCOL_WS2812B]     = &leds_protocol_ws2812b,
  [LEDS_PROTOCOL_SK6812_GRBW] = &leds_protocol_sk6812grbw,
  [LEDS_PROTOCOL_WS2811]      = &leds_protocol_ws2811,
  [LEDS_PROTOCOL_SK9822]      = &leds_protocol_sk9822,
};

const struct leds_protocol_type *leds_protocol_type(enum leds_protocol protocol)
{
  if (protocol < LEDS_PROTOCOLS_COUNT && leds_protocol_types[protocol]) {
    return leds_protocol_types[protocol];
  } else {
    LOG_FATAL("invalid protocol=%d", protocol);
  }
}
