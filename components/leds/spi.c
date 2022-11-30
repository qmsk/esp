#include <leds.h>
#include "leds.h"

#if CONFIG_LEDS_SPI_ENABLED
  size_t leds_spi_buffer_for_protocol(enum leds_protocol protocol, unsigned count)
  {
    const struct leds_protocol_type *protocol_type = leds_protocol_type(protocol);

    if (protocol_type->spi_interface_mode) {
      return leds_interface_spi_buffer_size(protocol_type->spi_interface_mode, count);
    } else {
      // not defined
      return 0;
    }
  }
#endif
