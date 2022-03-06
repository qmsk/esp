#pragma once

#include <sdkconfig.h>

#if CONFIG_LEDS_GPIO_ENABLED
# include <gpio_out.h>
#endif

#if CONFIG_LEDS_I2S_ENABLED
# include <i2s_out.h>
# define LEDS_I2S_GPIO_PIN_ENABLED I2S_OUT_OPTIONS_DATA_GPIO_ENABLED
#endif

#if CONFIG_LEDS_SPI_ENABLED && CONFIG_IDF_TARGET_ESP8266
// using custom spi_master driver
# include <spi_master.h>

#elif CONFIG_LEDS_SPI_ENABLED
// using esp-idf spi_master driver
# include <hal/spi_types.h>
#endif

#if CONFIG_LEDS_UART_ENABLED
# include <uart.h>
#endif

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <stdint.h>

struct leds;

enum leds_interface {
  /* Dummy, no-op TX */
  LEDS_INTERFACE_NONE,

#if CONFIG_LEDS_SPI_ENABLED
  /*
   * Supported protocols:
   *  - LEDS_PROTOCOL_APA102
   *  - LEDS_PROTOCOL_P9813
   */
  LEDS_INTERFACE_SPI            = 1,
#endif

#if CONFIG_LEDS_UART_ENABLED
  /* Supported protocols:
   *  - LEDS_PROTOCOL_WS2812B
   *  - LEDS_PROTOCOL_SK6812_GRBW
   *  - LEDS_PROTOCOL_WS2811
   */
  LEDS_INTERFACE_UART           = 2,
#endif

#if CONFIG_LEDS_I2S_ENABLED
  /* Supported protocols:
   *  - LEDS_PROTOCOL_WS2812B
   *  - LEDS_PROTOCOL_WS2811
   *  - LEDS_PROTOCOL_SK6812_GRBW
   */
  LEDS_INTERFACE_I2S           = 3,
#endif

  LEDS_INTERFACE_COUNT
};

#if CONFIG_LEDS_UART_ENABLED
# define LEDS_UART_RX_BUFFER_SIZE 0
# define LEDS_UART_TX_BUFFER_SIZE 512
#endif

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

#if CONFIG_LEDS_SPI_ENABLED
  /*
   * Returns total SPI data buffer sized required for protocol and count LEDs.
   *
   * @return 0 if not supported for protocol
   */
  size_t leds_spi_buffer_for_protocol(enum leds_protocol protocol, unsigned count);
#endif

#if CONFIG_LEDS_I2S_ENABLED
  /*
   * Returns total TX buffer sized required for protocol and count LEDs.
   *
   * @return 0 if not supported for protocol
   */
  size_t leds_i2s_buffer_for_protocol(enum leds_protocol protocol, unsigned count);
#endif

struct leds_options {
  enum leds_interface interface;
  enum leds_protocol protocol;

  unsigned count;

#if CONFIG_LEDS_SPI_ENABLED && CONFIG_IDF_TARGET_ESP8266
  /** LEDS_INTERFACE_SPI */
  struct spi_master *spi_master;
  enum spi_mode spi_mode_bits; /* Optional SPI mode bits to set in addition to protocol SPI_MODE_{0-4} */
  enum spi_clock spi_clock;
#elif CONFIG_LEDS_SPI_ENABLED
  /** LEDS_INTERFACE_SPI */
  spi_host_device_t spi_host;
  int spi_clock; /* Hz, divisible by APB_CLK_FREQ */
  int spi_cs_io; /* GPIO pin, -1 if not used */
  bool spi_cs_high; /* GPIO high during TX, default low */
#endif

#if CONFIG_LEDS_UART_ENABLED
  /** LEDS_INTERFACE_UART */
  struct uart *uart;
  SemaphoreHandle_t uart_pin_mutex;
#endif

#if CONFIG_LEDS_I2S_ENABLED
  /** LEDS_INTERFACE_I2S */
  struct i2s_out *i2s_out;
  SemaphoreHandle_t i2s_pin_mutex;
  TickType_t i2s_pin_timeout;
# if LEDS_I2S_GPIO_PIN_ENABLED
  gpio_num_t i2s_gpio_pin;
# endif
#endif

#if CONFIG_LEDS_GPIO_ENABLED
  /**
   * GPIO used for output multiplexing.
   * The `gpio_out_pins` will be set before TX start, and cleared after TX end.
   * Any other `gpio_out->pins` will be cleared.
   */
  struct gpio_out *gpio_out;
  enum gpio_out_pins gpio_out_pins;
#endif
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

/* Output frames on interface */
int leds_tx(struct leds *leds);
