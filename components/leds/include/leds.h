#ifndef __LEDS_H__
#define __LEDS_H__

#include <gpio_out.h>
#include <i2s_out.h>
#include <spi_master.h>
#include <uart.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <stdint.h>

struct leds;

enum leds_interface {
  /*
   * Supported protocols:
   *  - LEDS_PROTOCOL_APA102
   *  - LEDS_PROTOCOL_P9813
   */
  LEDS_INTERFACE_SPI            = 1,

  /* Supported protocols:
   *  - LEDS_PROTOCOL_WS2812B
   *  - LEDS_PROTOCOL_SK6812_GRBW
   *  - LEDS_PROTOCOL_WS2811
   */
  LEDS_INTERFACE_UART           = 2,

  /* Supported protocols:
   *  - LEDS_PROTOCOL_WS2812B
   *  - LEDS_PROTOCOL_WS2811
   *  - LEDS_PROTOCOL_SK6812_GRBW
   */
  LEDS_INTERFACE_I2S           = 3,
};

enum leds_protocol {
  LEDS_PROTOCOL_APA102          = 1,
  LEDS_PROTOCOL_P9813           = 2,
  LEDS_PROTOCOL_WS2812B         = 3,
  LEDS_PROTOCOL_SK6812_GRBW     = 4,
  LEDS_PROTOCOL_WS2811          = 5,
};

/* interpretation of spi_led_color.parameter by protocol */
enum leds_color_parameter {
  LEDS_COLOR_NONE         = 0,
  LEDS_COLOR_DIMMER,
  LEDS_COLOR_WHITE,
};

enum leds_format {
  LEDS_FORMAT_RGB,
  LEDS_FORMAT_BGR,
  LEDS_FORMAT_GRB,
  LEDS_FORMAT_RGBA,
  LEDS_FORMAT_RGBW,
};

struct leds_format_params {
  /* Limit number of LED (segments) to read */
  unsigned count;

  /* Set LEDs starting at offset */
  unsigned offset;

  /* Set segments of multiple consecutive LEDs per channel */
  unsigned segment;
};

/*
 * Returns preferred interface for given protocol.
 *
 * ATM only one interface is supported by each of the protocols...
 */
enum leds_interface leds_interface_for_protocol(enum leds_protocol protocol);

/*
 * Returns total TX buffer sized required for protocol and count LEDs.
 *
 * @return 0 if not supported for protocol
 */
size_t leds_i2s_buffer_for_protocol(enum leds_protocol protocol, unsigned count);

struct leds_options {
  enum leds_interface interface;
  enum leds_protocol protocol;

  unsigned count;

  /** LEDS_INTERFACE_SPI */
  struct spi_master *spi_master;
  enum spi_mode spi_mode_bits; /* Optional SPI mode bits to set in addition to protocol SPI_MODE_{0-4} */
  enum spi_clock spi_clock;

  /** LEDS_INTERFACE_UART */
  struct uart *uart;
  SemaphoreHandle_t uart_pin_mutex;

  /** LEDS_INTERFACE_I2S */
  struct i2s_out *i2s_out;
  SemaphoreHandle_t i2s_pin_mutex;

  /** GPIO for output multiplexing */
  struct gpio_out *gpio_out;
  enum gpio_out_pins gpio_out_pins;
};

/*
 * Returns leds_color.parameter interpretation for protocol.
 */
enum leds_color_parameter leds_color_parameter_for_protocol(enum leds_protocol protocol);

/*
 * Returns default leds_color.parameter to use for protocol.
 */
uint8_t leds_default_color_parameter_for_protocol(enum leds_protocol protocol);

struct spi_led_color {
  uint8_t r, g, b;

  union {
    uint8_t parameter;
    uint8_t dimmer; // 0-255
    uint8_t white; // 0-255
  };
};

enum leds_test_mode {
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

  TEST_MODE_END
};

int leds_new(struct leds **ledsp, const struct leds_options *options);

/* Get options */
const struct leds_options *leds_options(struct leds *leds);

enum leds_protocol leds_protocol(struct leds *leds);

enum leds_interface leds_interface(struct leds *leds);

unsigned leds_count(struct leds *leds);

/* Get active LED count */
unsigned leds_active(struct leds *leds);

/*
 * @param index 0-based index
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int leds_set(struct leds *leds, unsigned index, struct spi_led_color color);

/*
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int leds_set_all(struct leds *leds, struct spi_led_color color);

/*
 * Decode LED colors from binary data, using given format.
 *
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int leds_set_format(struct leds *leds, enum leds_format format, void *data, size_t len, struct leds_format_params params);

/*
 * Set test pattern for mode/tick. Requires `leds_tx()`.
 *
 * Returns number of ticks to hold this frame, 0 for last frame, <0 on error.
 */
int leds_set_test(struct leds *leds, enum leds_test_mode mode, unsigned frame);

/* Send frames on output interface */
int leds_tx(struct leds *leds);

#endif
