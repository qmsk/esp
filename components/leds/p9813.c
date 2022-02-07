#include "p9813.h"
#include "leds.h"
#include "spi.h"

#include <logging.h>

#include <stdlib.h>

#define P9813_SPI_MODE (SPI_MODE_0)

#define P9813_START_FRAME (struct p9813_frame){ 0x00, 0x00, 0x00, 0x00 }
#define P9813_STOP_FRAME (struct p9813_frame){ 0x00, 0x00, 0x00, 0x00 }

#define P9813_CONTROL_BYTE(b, g, r) (0xC0 | ((~(b) & 0xC0) >> 2) | ((~(g) & 0xC0) >> 4) | ((~(r) & 0xC0) >> 6))

static int p9813_new_packet(struct p9813_packet **packetp, size_t *sizep, unsigned count)
{
  unsigned stopframes = 1; // single 32-bit frame3
  size_t size = (1 + count + stopframes) * sizeof(struct p9813_frame);
  struct p9813_packet *packet;

  if (!(packet = malloc(size))) {
    LOG_ERROR("malloc");
    return -1;
  }

  *packetp = packet;
  *sizep = size;

  // frames
  packet->start = P9813_START_FRAME;

  for (unsigned i = 0; i < count; i++) {
    packet->frames[i] = (struct p9813_frame){ P9813_CONTROL_BYTE(0, 0, 0), 0, 0, 0 }; // off
  }

  for (unsigned i = count; i < count + stopframes; i++) {
    packet->frames[i] = P9813_STOP_FRAME;
  }

  return 0;
}

int leds_init_p9813(struct leds_protocol_p9813 *protocol, const struct leds_options *options)
{
  return p9813_new_packet(&protocol->packet, &protocol->packet_size, options->count);
}

int leds_tx_p9813(struct leds_protocol_p9813 *protocol, const struct leds_options *options)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      return leds_tx_spi(options, P9813_SPI_MODE, protocol->packet, protocol->packet_size);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void p9813_set_frame(struct leds_protocol_p9813 *protocol, unsigned index, struct spi_led_color color)
{
  protocol->packet->frames[index] = (struct p9813_frame) {
    .control = P9813_CONTROL_BYTE(color.b, color.g, color.r),
    .b = color.b,
    .g = color.g,
    .r = color.r,
  };
}

void p9813_set_frames(struct leds_protocol_p9813 *protocol, unsigned count, struct spi_led_color color)
{
  for (unsigned index = 0; index < count; index++) {
    protocol->packet->frames[index] = (struct p9813_frame) {
      .control = P9813_CONTROL_BYTE(color.b, color.g, color.r),
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

static inline bool p9813_frame_active(const struct p9813_frame frame)
{
  return frame.b || frame.g || frame.r;
}

unsigned p9813_count_active(struct leds_protocol_p9813 *protocol, unsigned count)
{
  unsigned active = 0;

  for (unsigned index = 0; index < count; index++) {
    if (p9813_frame_active(protocol->packet->frames[index])) {
      active++;
    }
  }

  return active;
}
