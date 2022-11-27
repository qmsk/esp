#include <leds.h>

#if CONFIG_LEDS_SPI_ENABLED
  size_t leds_spi_buffer_for_protocol(enum leds_protocol protocol, unsigned count)
  {
    switch (protocol) {
      case LEDS_PROTOCOL_APA102:
        return leds_protocol_apa102_spi_buffer_size(count);

      case LEDS_PROTOCOL_P9813:
        return leds_protocol_p9813_spi_buffer_size(count);

      default:
        // unknown
        return 0;
    }
  }
#endif
