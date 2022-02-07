#include "apa102.h"
#include "leds.h"

#include <logging.h>

#include <stdlib.h>

#define APA102_SPI_MODE (SPI_MODE_3)

#define APA102_START_FRAME (struct apa102_frame){ 0x00, 0x00, 0x00, 0x00 }
#define APA102_STOP_FRAME (struct apa102_frame){ 0x00, 0x00, 0x00, 0x00 }

#define APA102_GLOBAL_BYTE(brightness) (0xE0 | ((brightness) >> 3))

static int apa102_new_packet(struct apa102_packet **packetp, size_t *sizep, unsigned count)
{
  unsigned stopframes = (1 + count / 32); // one bit per LED, in frames of 32 bits
  size_t size = (1 + count + stopframes) * sizeof(struct apa102_frame);
  struct apa102_packet *packet;

  if (!(packet = malloc(size))) {
    LOG_ERROR("malloc");
    return -1;
  }

  *packetp = packet;
  *sizep = size;

  // frames
  packet->start = APA102_START_FRAME;

  for (unsigned i = 0; i < count; i++) {
    packet->frames[i] = (struct apa102_frame){ APA102_GLOBAL_BYTE(0), 0, 0, 0 }; // off
  }

  for (unsigned i = count; i < count + stopframes; i++) {
    packet->frames[i] = APA102_STOP_FRAME;
  }

  return 0;
}

int leds_init_apa102(struct leds_protocol_apa102 *protocol, const struct leds_options *options)
{
  return apa102_new_packet(&protocol->packet, &protocol->packet_size, options->count);
}

int leds_tx_apa102(struct leds_protocol_apa102 *protocol, const struct leds_options *options)
{
  switch (options->interface) {
    case LEDS_INTERFACE_SPI:
      return leds_tx_spi(options, APA102_SPI_MODE, protocol->packet, protocol->packet_size);

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return 1;
  }
}

void apa102_set_frame(struct leds_protocol_apa102 *protocol, unsigned index, struct spi_led_color color)
{
  protocol->packet->frames[index] = (struct apa102_frame) {
    .global = APA102_GLOBAL_BYTE(color.dimmer),
    .b = color.b,
    .g = color.g,
    .r = color.r,
  };
}

void apa102_set_frames(struct leds_protocol_apa102 *protocol, unsigned count, struct spi_led_color color)
{
  for (unsigned index = 0; index < count; index++) {
    protocol->packet->frames[index] = (struct apa102_frame) {
      .global = APA102_GLOBAL_BYTE(color.dimmer),
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

static inline bool apa102_frame_active(const struct apa102_frame frame)
{
  return (frame.b || frame.g || frame.r) && frame.global > APA102_GLOBAL_BYTE(0);
}

unsigned apa102_count_active(struct leds_protocol_apa102 *protocol, unsigned count)
{
  unsigned active = 0;

  for (unsigned index = 0; index < count; index++) {
    if (apa102_frame_active(protocol->packet->frames[index])) {
      active++;
    }
  }

  return active;
}
