#pragma once

#include <sdkconfig.h>

#if CONFIG_LEDS_GPIO_ENABLED
# include <gpio_out.h>
#endif

#if CONFIG_LEDS_I2S_ENABLED
# include <i2s_out.h>
# define LEDS_I2S_GPIO_PINS_ENABLED I2S_OUT_GPIO_PINS_SUPPORTED
# define LEDS_I2S_DATA_PINS_ENABLED I2S_OUT_PARALLEL_SUPPORTED
# define LEDS_I2S_DATA_PINS_SIZE I2S_OUT_PARALLEL_SIZE
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

#define LEDS_COUNT_MAX 65535 // 16-bit

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
   *  - LEDS_PROTOCOL_SK9822
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
  LEDS_PROTOCOL_NONE,

  LEDS_PROTOCOL_APA102,
  LEDS_PROTOCOL_P9813,

  LEDS_PROTOCOL_WS2812B,
  LEDS_PROTOCOL_SK6812_GRBW,
  LEDS_PROTOCOL_WS2811,
  LEDS_PROTOCOL_SK9822,
};

/* interpretation of leds_color.parameter by protocol */
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

#if CONFIG_LEDS_GPIO_ENABLED
  struct leds_interface_options_gpio {
    /**
     * GPIO used for output multiplexing.
     * The `gpio_out_pins` will be set before TX start, and cleared after TX end.
     * Any other `gpio_out->pins` will be cleared.
     */
    struct gpio_out *gpio_out;

    gpio_out_pins_t gpio_out_pins;
  };
#endif

#if CONFIG_LEDS_SPI_ENABLED
  struct leds_interface_spi_options {
  #if CONFIG_LEDS_GPIO_ENABLED
    struct leds_interface_options_gpio gpio;
  #endif

  #if CONFIG_IDF_TARGET_ESP8266
    struct spi_master *spi_master;

    /* Optional SPI mode bits to set in addition to protocol SPI_MODE_{0-4} */
    enum spi_mode mode_bits;

    enum spi_clock clock;
  #else
    spi_host_device_t host;

    /* Hz, divisible by APB_CLK_FREQ */
    int clock;

    /* GPIO pin */
    int cs_io; // -1 if not used

    /* GPIO high during TX, default low */
    bool cs_high;
  #endif
  };

  /*
   * Returns total SPI data buffer sized required for protocol and count LEDs.
   *
   * @return 0 if not supported for protocol
   */
  size_t leds_spi_buffer_for_protocol(enum leds_protocol protocol, unsigned count);
#endif

#if CONFIG_LEDS_UART_ENABLED
  struct leds_interface_uart_options {
  #if CONFIG_LEDS_GPIO_ENABLED
    struct leds_interface_options_gpio gpio;
  #endif

    struct uart *uart;

    SemaphoreHandle_t pin_mutex;
  };
#endif

#if CONFIG_LEDS_I2S_ENABLED
  struct leds_interface_i2s_options {
  #if CONFIG_LEDS_GPIO_ENABLED
    struct leds_interface_options_gpio gpio;
  #endif

    struct i2s_out *i2s_out;

    SemaphoreHandle_t pin_mutex;

    TickType_t pin_timeout;

  #if LEDS_I2S_GPIO_PINS_ENABLED
  # if LEDS_I2S_DATA_PINS_ENABLED
    unsigned data_pins_count; // default 0 -> serial output with a single data_pin
  # endif

    // use GPIO_NUM_NC < 0 to disable
    union {
      gpio_num_t data_pin; // for pin_count == 0 -> serial output
    #if LEDS_I2S_DATA_PINS_ENABLED
      gpio_num_t data_pins[LEDS_I2S_DATA_PINS_SIZE]; // for pin_count >= 1 -> parallel output
    #endif
    };

    // use GPIO_NUM_NC < 0 to disable
    union {
      gpio_num_t inv_data_pin; // for pin_count == 0 -> serial output
    #if LEDS_I2S_DATA_PINS_ENABLED
      gpio_num_t inv_data_pins[LEDS_I2S_DATA_PINS_SIZE]; // for pin_count >= 1 -> parallel output
    #endif
    };
  #endif

    // only used for protocols with separate clock/data lines
    int clock_rate;
  #if LEDS_I2S_GPIO_PINS_ENABLED
    gpio_num_t clock_pin;
  #endif
  };

  /*
   * Returns total TX buffer size, align required for protocol with `led_count` LEDs on one serial pin.
   *
   * @return 0 if not supported for protocol
   */
  size_t leds_i2s_serial_buffer_size(enum leds_protocol protocol, unsigned led_count);
  size_t leds_i2s_serial_buffer_align(enum leds_protocol protocol);

  /*
   * Returns total TX buffer size/align required for protocol with `led_count` LEDs across `pin_count` parallel pins.
   *
   * @return 0 if not supported for protocol
   */
  size_t leds_i2s_parallel_buffer_size(enum leds_protocol protocol, unsigned led_count, unsigned pin_count);
  size_t leds_i2s_parallel_buffer_align(enum leds_protocol protocol, unsigned pin_count);

#endif

struct leds_options {
  enum leds_interface interface;
  enum leds_protocol protocol;

  unsigned count;

  /* Limit number of active LEDs by color. Going over the limit will scale down all LEDs */
  unsigned limit;

  /* By interface */
  union {
#if CONFIG_LEDS_SPI_ENABLED
    struct leds_interface_spi_options spi;
#endif
#if CONFIG_LEDS_UART_ENABLED
    struct leds_interface_uart_options uart;
#endif
#if CONFIG_LEDS_I2S_ENABLED
    struct leds_interface_i2s_options i2s;
#endif
  };
};

/*
 * Returns leds_color.parameter interpretation for protocol.
 */
enum leds_color_parameter leds_color_parameter_for_protocol(enum leds_protocol protocol);

/*
 * Returns default leds_color.parameter to use for protocol.
 */
uint8_t leds_default_color_parameter_for_protocol(enum leds_protocol protocol);

struct leds_color {
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

/*
 * Set all LEDs off.
 */
int leds_clear_all(struct leds *leds);

/*
 * @param index 0-based index
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int leds_set(struct leds *leds, unsigned index, struct leds_color color);

/*
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int leds_set_all(struct leds *leds, struct leds_color color);

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

/*
 * Return number of active LEDs, with non-zero color.
 */
unsigned leds_count_active(struct leds *leds);

/*
 * Return total power for leds, ref options->limit.
 */
unsigned leds_count_total(struct leds *leds);

/* Output frames on interface */
int leds_tx(struct leds *leds);

/*
 * Get power limit utilization from last leds_tx() operation.
 * *
 * @return 0.0 .. 1.0 .. +inf
 *         0.0 -> all LEDs are off, or no limit configured
 *         1.0 -> at power limit
 *        >1.0 -> power limiting active
 */
float leds_limit_utilization(struct leds *leds);
/*
 * Get power limit applied to last leds_tx() operation.
 *
 * This is applied using integer math internally, the floating point value is an approximation.
 *
 * @return 0.0 .. 1.0, with 1.0 meaning power is not being limited and numbers closer to 0 meaning more power limiting.
 */
float leds_limit_active(struct leds *leds);
