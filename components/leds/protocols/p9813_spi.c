#include "p9813.h"
#include "../leds.h"
#include "../interfaces/spi.h"

#include <logging.h>

#if CONFIG_LEDS_SPI_ENABLED
  #define LEDS_PROTOCOL_P9813_INTERFACE_SPI_MODE LEDS_INTERFACE_SPI_MODE0_32BIT

  void leds_protocol_p9813_spi_out(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union p9813_pixel pixel = p9813_pixel(pixels[index], index, limit);

    // 32-bit little-endian
    buf[0] = pixel.xbgr;
  }
#endif
