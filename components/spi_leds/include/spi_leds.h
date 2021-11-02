#ifndef __SPI_LEDS_H__
#define __SPI_LEDS_H__

#include <gpio_out.h>
#include <spi_master.h>
#include <uart1.h>

#include <stdint.h>

struct spi_leds;

enum spi_leds_interface {
  /*
   * Supported protocols:
   *  - SPI_LEDS_PROTOCOL_APA102
   *  - SPI_LEDS_PROTOCOL_P9813
   */
  SPI_LEDS_INTERFACE_SPI            = 1,

  /* Supported protocols:
   *  - SPI_LEDS_PROTOCOL_WS2812B
   */
  SPI_LEDS_INTERFACE_UART           = 2,
};

enum spi_leds_protocol {
  SPI_LEDS_PROTOCOL_APA102          = 1,
  SPI_LEDS_PROTOCOL_P9813           = 2,
  SPI_LEDS_PROTOCOL_WS2812B         = 3,
  SPI_LEDS_PROTOCOL_SK6812_GRBW     = 4,
};

struct spi_leds_options {
  enum spi_leds_interface interface;
  enum spi_leds_protocol protocol;

  unsigned count;

  /** SPI_LEDS_INTERFACE_SPI */
  struct spi_master *spi_master;
  enum spi_mode spi_mode_bits; /* Optional SPI mode bits to set in addition to protocol SPI_MODE_{0-4} */
  enum spi_clock spi_clock;

  /** SPI_LEDS_INTERFACE_UART */
  struct uart1 *uart1;

  /** GPIO for output multiplexing */
  struct gpio_out *gpio_out;
  enum gpio_out_pins gpio_out_pins;
};

struct spi_led_color {
  uint8_t r, g, b;

  union {
    uint8_t parameter;
    uint8_t brightness; // optional, 0-255
    uint8_t white; // 0-255
  };
};

enum spi_leds_test_mode {
  TEST_MODE_BLACK,
  TEST_MODE_CHASE,

  TEST_MODE_BLACK_RED,
  TEST_MODE_RED_YELLOW,
  TEST_MODE_YELLOW_GREEN,
  TEST_MODE_GREEN_CYAN,
  TEST_MODE_CYAN_BLUE,
  TEST_MODE_BLUE_MAGENTA,
  TEST_MODE_MAGENTA_RED,
  TEST_MODE_RED_BLACK,

  TEST_MODE_MAX
};

int spi_leds_new(struct spi_leds **spi_ledsp, const struct spi_leds_options *options);

/* Get LED count */
unsigned spi_leds_count(struct spi_leds *spi_leds);

/* Get active LED count */
unsigned spi_leds_active(struct spi_leds *spi_leds);

/*
 * @param index 0-based index
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int spi_leds_set(struct spi_leds *spi_leds, unsigned index, struct spi_led_color color);

/*
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int spi_leds_set_all(struct spi_leds *spi_leds, struct spi_led_color color);

/* Send frames on output interface */
int spi_leds_tx(struct spi_leds *spi_leds);

/*
 * Output animated test patterns.
 *
 * Blocks task until complete.
 */
int spi_leds_test(struct spi_leds *spi_leds, enum spi_leds_test_mode mode);

#endif
