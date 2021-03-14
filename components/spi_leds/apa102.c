#include "apa102.h"

#include <logging.h>

#include <stdlib.h>

#define APA102_START_FRAME (struct apa102_frame){ 0x00, 0x00, 0x00, 0x00 }
#define APA102_STOP_FRAME (struct apa102_frame){ 0x00, 0x00, 0x00, 0x00 }

#define APA102_GLOBAL_BYTE(brightness) (0xE0 | ((brightness) >> 3))

int apa102_new_packet(struct apa102_packet **packetp, size_t *sizep, unsigned count)
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

void apa102_set_frame(struct apa102_packet *packet, unsigned index, struct spi_led_color color)
{
  packet->frames[index] = (struct apa102_frame) {
    .global = APA102_GLOBAL_BYTE(color.parameters.brightness),
    .b = color.b,
    .g = color.g,
    .r = color.r,
  };
}

void apa102_set_frames(struct apa102_packet *packet, unsigned count, struct spi_led_color color)
{
  for (unsigned index = 0; index < count; index++) {
    packet->frames[index] = (struct apa102_frame) {
      .global = APA102_GLOBAL_BYTE(color.parameters.brightness),
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}
