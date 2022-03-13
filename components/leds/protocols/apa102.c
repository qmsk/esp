#include "apa102.h"
#include "../leds.h"
#include "../limit.h"

#include <logging.h>

#include <stdlib.h>

#if CONFIG_LEDS_SPI_ENABLED
# define APA102_SPI_MODE (SPI_MODE_3)
#endif

struct __attribute__((packed)) apa102_frame {
  uint8_t global;
  uint8_t b, g, r;
};

struct __attribute__((packed)) apa102_packet {
  struct apa102_frame start; // zero
  struct apa102_frame frames[];
};

#define APA102_START_FRAME (struct apa102_frame){ 0x00, 0x00, 0x00, 0x00 }
#define APA102_STOP_FRAME (struct apa102_frame){ 0x00, 0x00, 0x00, 0x00 }

#define APA102_GLOBAL_BYTE(brightness) (0xE0 | ((brightness) >> 3))
#define APA102_BRIGHTNESS(global) ((global & 0x1F) << 3) // 0..255

#define APA102_FRAME_TOTAL_DIVISOR (3 * 255 * 31) // one frame at full brightness

static size_t apa102_packet_size(unsigned count)
{
  unsigned stopframes = (1 + count / 32); // one bit per LED, in frames of 32 bits

  return (1 + count + stopframes) * sizeof(struct apa102_frame);
}

static void apa102_packet_init(struct apa102_packet *packet, unsigned count)
{
  unsigned stopframes = (1 + count / 32); // one bit per LED, in frames of 32 bits

  // frames
  packet->start = APA102_START_FRAME;

  for (unsigned i = 0; i < count; i++) {
    packet->frames[i] = (struct apa102_frame){ APA102_GLOBAL_BYTE(0), 0, 0, 0 }; // off
  }

  for (unsigned i = count; i < count + stopframes; i++) {
    packet->frames[i] = APA102_STOP_FRAME;
  }
}

static inline bool apa102_frame_active(const struct apa102_frame frame)
{
  return (frame.b || frame.g || frame.r) && frame.global > APA102_GLOBAL_BYTE(0);
}

static inline unsigned apa102_frame_total(const struct apa102_frame frame)
{
  // scale 0..255 brightness to 0..31 to not overflow a 32-bit uint for a 16-bit LEDS_COUNT_MAX
  return (frame.b + frame.r + frame.g) * (APA102_BRIGHTNESS(frame.global) >> 3);
}

static inline struct apa102_frame apa102_frame_limit(const struct apa102_frame frame, struct leds_limit limit)
{
  return (struct apa102_frame) {
    .b  = leds_limit_uint8(limit, frame.b),
    .g  = leds_limit_uint8(limit, frame.g),
    .r  = leds_limit_uint8(limit, frame.r),
  };
}

size_t leds_protocol_apa102_spi_buffer_size(unsigned count)
{
  return apa102_packet_size(count);
}

int leds_protocol_apa102_init(struct leds_protocol_apa102 *protocol, union leds_interface_state *interface, const struct leds_options *options)
{
  void *buf;
  size_t size = apa102_packet_size(options->count);
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
      if ((err = leds_interface_spi_init(&interface->spi, options, &buf, size, APA102_SPI_MODE))) {
        LOG_ERROR("leds_interface_spi_init");
        return err;
      }

      break;
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }

  protocol->packet = buf;
  protocol->packet_size = size;
  protocol->count = options->count;

  apa102_packet_init(protocol->packet, protocol->count);

  return 0;
}

int leds_protocol_apa102_tx(struct leds_protocol_apa102 *protocol, union leds_interface_state *interface, const struct leds_options *options)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      return leds_interface_spi_tx(&interface->spi, options, protocol->packet, protocol->packet_size);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void leds_protocol_apa102_set(struct leds_protocol_apa102 *protocol, unsigned index, struct leds_color color)
{
  if (index < protocol->count) {
    protocol->packet->frames[index] = (struct apa102_frame) {
      .global = APA102_GLOBAL_BYTE(color.dimmer),
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

void leds_protocol_apa102_set_all(struct leds_protocol_apa102 *protocol, struct leds_color color)
{
  for (unsigned index = 0; index < protocol->count; index++) {
    protocol->packet->frames[index] = (struct apa102_frame) {
      .global = APA102_GLOBAL_BYTE(color.dimmer),
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

unsigned leds_protocol_apa102_count_active(struct leds_protocol_apa102 *protocol)
{
  unsigned active = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    if (apa102_frame_active(protocol->packet->frames[index])) {
      active++;
    }
  }

  return active;
}

unsigned leds_protocol_apa102_count_total(struct leds_protocol_apa102 *protocol)
{
  unsigned total = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    total += apa102_frame_total(protocol->packet->frames[index]);
  }

  return total / APA102_FRAME_TOTAL_DIVISOR;
}
