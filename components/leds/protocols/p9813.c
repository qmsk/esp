#include "p9813.h"
#include "../leds.h"
#include "../limit.h"

#include <logging.h>

#include <stdlib.h>

#if CONFIG_LEDS_SPI_ENABLED
# define P9813_SPI_MODE (SPI_MODE_0)
#endif

struct __attribute__((packed)) p9813_frame {
  uint8_t control;
  uint8_t b, g, r;
};

struct __attribute__((packed)) p9813_packet {
  struct p9813_frame start; // zero
  struct p9813_frame frames[];
};

#define P9813_START_FRAME (struct p9813_frame){ 0x00, 0x00, 0x00, 0x00 }
#define P9813_STOP_FRAME (struct p9813_frame){ 0x00, 0x00, 0x00, 0x00 }

#define P9813_CONTROL_BYTE(b, g, r) (0xC0 | ((~(b) & 0xC0) >> 2) | ((~(g) & 0xC0) >> 4) | ((~(r) & 0xC0) >> 6))

#define P9813_FRAME_POWER_DIVISOR (3 * 255) // one frame at full brightness

static size_t p9813_packet_size(unsigned count)
{
  unsigned stopframes = 1; // single 32-bit frame

  return (1 + count + stopframes) * sizeof(struct p9813_frame);
}

static void p9813_packet_init(struct p9813_packet *packet, unsigned count)
{
  unsigned stopframes = 1; // single 32-bit frame

  // frames
  packet->start = P9813_START_FRAME;

  for (unsigned i = 0; i < count; i++) {
    packet->frames[i] = (struct p9813_frame){ P9813_CONTROL_BYTE(0, 0, 0), 0, 0, 0 }; // off
  }

  for (unsigned i = count; i < count + stopframes; i++) {
    packet->frames[i] = P9813_STOP_FRAME;
  }
}

static inline bool p9813_frame_active(const struct p9813_frame frame)
{
  return frame.b || frame.g || frame.r;
}

static inline unsigned p9813_frame_power(const struct p9813_frame frame)
{
  return frame.b + frame.r + frame.g;
}

static inline struct p9813_frame p9813_frame_limit(const struct p9813_frame frame, struct leds_limit limit)
{
  return (struct p9813_frame) {
    .b  = leds_limit_uint8(limit, frame.b),
    .g  = leds_limit_uint8(limit, frame.g),
    .r  = leds_limit_uint8(limit, frame.r),
  };
}

size_t leds_protocol_p9813_spi_buffer_size(unsigned count)
{
  return p9813_packet_size(count);
}

int leds_protocol_p9813_init(struct leds_protocol_p9813 *protocol, union leds_interface_state *interface, const struct leds_options *options)
{
  void *buf;
  size_t size = p9813_packet_size(options->count);
  int err;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      if (!(buf = malloc(size))) {
        LOG_ERROR("malloc");
        return -1;
      }
      break;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      if ((err = leds_interface_spi_init(&interface->spi, &options->spi, &buf, size, P9813_SPI_MODE))) {
        LOG_ERROR("leds_interface_spi_init");
        return err;
      }

      break;
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
  }

  protocol->packet = buf;
  protocol->packet_size = size;
  protocol->count = options->count;

  p9813_packet_init(protocol->packet, protocol->count);

  return 0;
}

int leds_protocol_p9813_tx(struct leds_protocol_p9813 *protocol, union leds_interface_state *interface, const struct leds_options *options)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      return leds_interface_spi_tx(&interface->spi, &options->spi, protocol->packet, protocol->packet_size);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void leds_protocol_p9813_set(struct leds_protocol_p9813 *protocol, unsigned index, struct leds_color color)
{
  if (index < protocol->count) {
    protocol->packet->frames[index] = (struct p9813_frame) {
      .control = P9813_CONTROL_BYTE(color.b, color.g, color.r),
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

void leds_protocol_p9813_set_all(struct leds_protocol_p9813 *protocol, struct leds_color color)
{
  for (unsigned index = 0; index < protocol->count; index++) {
    protocol->packet->frames[index] = (struct p9813_frame) {
      .control = P9813_CONTROL_BYTE(color.b, color.g, color.r),
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

unsigned leds_protocol_p9813_count_active(struct leds_protocol_p9813 *protocol)
{
  unsigned active = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    if (p9813_frame_active(protocol->packet->frames[index])) {
      active++;
    }
  }

  return active;
}

unsigned leds_protocol_p9813_count_power(struct leds_protocol_p9813 *protocol, unsigned index, unsigned count)
{
  unsigned power = 0;

  for (unsigned i = index; i < index + count; i++) {
    power += p9813_frame_power(protocol->packet->frames[i]);
  }

  return power / P9813_FRAME_POWER_DIVISOR;
}
