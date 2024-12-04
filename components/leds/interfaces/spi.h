#pragma once

#include <leds.h>

#include "../limit.h"

#if CONFIG_LEDS_SPI_ENABLED
  #if CONFIG_IDF_TARGET_ESP8266
    // using custom spi_master driver
    #include <spi_master.h>

    #define LEDS_INTERFACE_SPI_MIN_SIZE (SPI_WRITE_MAX)
    #define LEDS_INTERFACE_SPI_MAX_SIZE (SPI_WRITE_MAX)

  #else
    // using esp-idf spi_master driver
    #include <driver/spi_master.h>

    #define LEDS_INTERFACE_SPI_MIN_SIZE (SOC_SPI_MAXIMUM_BUFFER_SIZE) // hardware FIFO
    #define LEDS_INTERFACE_SPI_MAX_SIZE (SPI_MAX_DMA_LEN) // assuming DMA

  #endif

  enum leds_interface_spi_mode {
    LEDS_INTERFACE_SPI_MODE_NONE = 0,

    // 32-bit frames, 32 x 0-bit start frame + 32 x 0-bit end frame with at least one bit per pixel
    LEDS_INTERFACE_SPI_MODE0_32BIT,
    LEDS_INTERFACE_SPI_MODE1_32BIT,
    LEDS_INTERFACE_SPI_MODE2_32BIT,
    LEDS_INTERFACE_SPI_MODE3_32BIT,
  };

  #define LEDS_INTERFACE_SPI_MODE_32BIT_START_FRAME 0x00000000
  #define LEDS_INTERFACE_SPI_MODE_32BIT_END_FRAME 0x00000000

  /* number of 32-bit end frames to ensure at least 1 bit per pixel */
  static inline unsigned leds_interface_spi_mode_32bit_end_frames(unsigned count) {
    return 1 + count / 32;
  }

  union leds_interface_spi_func {
    void (*spi_mode_32bit)(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
  };

  union leds_interface_spi_buf {
    void *p;
    uint8_t *u8;
    uint32_t *spi_mode_32bit;
  };

  #define LEDS_INTERFACE_SPI_FUNC(type, func) ((union leds_interface_spi_func) { .type = func })

  struct leds_interface_spi {
    enum leds_interface_spi_mode mode;
    union leds_interface_spi_func func;
    union leds_interface_spi_buf buf;

    size_t buf_size;

  #if CONFIG_IDF_TARGET_ESP8266
    struct spi_master *spi_master;
    struct spi_write_options spi_write_options;
  #else
    spi_device_handle_t device;
  #endif

    struct leds_interface_options_gpio gpio;
  };

  size_t leds_interface_spi_buffer_size(enum leds_interface_spi_mode mode, unsigned count);

  int leds_interface_spi_init(struct leds_interface_spi *interface, const struct leds_interface_spi_options *options, enum leds_interface_spi_mode mode, union leds_interface_spi_func func);
  int leds_interface_spi_tx(struct leds_interface_spi *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit);
#endif
