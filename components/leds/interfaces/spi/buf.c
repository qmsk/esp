#include "../spi.h"

#include <logging.h>

size_t leds_interface_spi_buf_size(enum leds_interface_spi_mode mode)
{
  switch (mode) {
    case LEDS_INTERFACE_SPI_MODE0_32BIT:
    case LEDS_INTERFACE_SPI_MODE1_32BIT:
    case LEDS_INTERFACE_SPI_MODE2_32BIT:
    case LEDS_INTERFACE_SPI_MODE3_32BIT:
      return sizeof(uint32_t);

    default:
      LOG_FATAL("invalid mode=%d", mode);
  }
}

unsigned leds_interface_spi_buf_count(enum leds_interface_spi_mode mode, unsigned count)
{
  switch (mode) {
    case LEDS_INTERFACE_SPI_MODE0_32BIT:
    case LEDS_INTERFACE_SPI_MODE1_32BIT:
    case LEDS_INTERFACE_SPI_MODE2_32BIT:
    case LEDS_INTERFACE_SPI_MODE3_32BIT:
      return 1 + count + leds_interface_spi_mode_32bit_end_frames(count);

    default:
      LOG_FATAL("invalid mode=%d", mode);
  }
}

size_t leds_interface_spi_buffer_size(enum leds_interface_spi_mode mode, unsigned count)
{
  size_t size = leds_interface_spi_buf_count(mode, count) * leds_interface_spi_buf_size(mode);

  if (size < LEDS_INTERFACE_SPI_MIN_SIZE) {
    return LEDS_INTERFACE_SPI_MIN_SIZE;
  } else if (size > LEDS_INTERFACE_SPI_MAX_SIZE) {
    return LEDS_INTERFACE_SPI_MAX_SIZE;
  } else {
    return size;
  }
}
