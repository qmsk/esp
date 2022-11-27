#include "apa102.h"
#include "../leds.h"
#include "../interfaces/spi.h"

#include <logging.h>

#if CONFIG_LEDS_SPI_ENABLED
  #define LEDS_PROTOCOL_APA102_INTERFACE_SPI_MODE LEDS_INTERFACE_SPI_MODE3_32BIT

  void leds_protocol_apa102_spi_out(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union apa102_pixel pixel = apa102_pixel(pixels[index], index, limit);

    // 32-bit little-endian
    buf[0] = pixel.xbgr;
  }
#endif
