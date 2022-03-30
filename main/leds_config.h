#pragma once

#include <config.h>
#include <leds.h>

#include <stdbool.h>

struct leds_state *state;

#if CONFIG_LEDS_GPIO_ENABLED
  #define LEDS_GPIO_SIZE 4

  enum leds_gpio_mode {
    LEDS_GPIO_MODE_DISABLED = -1,
    LEDS_GPIO_MODE_LOW      = 0,
    LEDS_GPIO_MODE_HIGH     = 1,
  };

  extern const struct config_enum leds_gpio_mode_enum[];
#endif

#if CONFIG_LEDS_SPI_ENABLED && CONFIG_IDF_TARGET_ESP8266

  #define SPI_CLOCK_DEFAULT (SPI_CLOCK_1MHZ)

  struct leds_spi_config {
    int host;
  };

#elif CONFIG_LEDS_SPI_ENABLED
  #define SPI_CLOCK_20MHZ   (APB_CLK_FREQ / 4)
  #define SPI_CLOCK_10MHZ   (APB_CLK_FREQ / 8)
  #define SPI_CLOCK_5MHZ    (APB_CLK_FREQ / 16)
  #define SPI_CLOCK_4MHZ    (APB_CLK_FREQ / 20)
  #define SPI_CLOCK_2MHZ    (APB_CLK_FREQ / 40)
  #define SPI_CLOCK_1MHZ    (APB_CLK_FREQ / 80)
  #define SPI_CLOCK_500KHZ  (APB_CLK_FREQ / 160)
  #define SPI_CLOCK_200KHZ  (APB_CLK_FREQ / 400)
  #define SPI_CLOCK_100KHZ  (APB_CLK_FREQ / 800)
  #define SPI_CLOCK_50KHZ   (APB_CLK_FREQ / 1600)
  #define SPI_CLOCK_20KHZ   (APB_CLK_FREQ / 4000)
  #define SPI_CLOCK_10KHZ   (APB_CLK_FREQ / 8000)
  #define SPI_CLOCK_1KHZ    (APB_CLK_FREQ / 80000)

  #define SPI_CLOCK_DEFAULT (SPI_CLOCK_1MHZ)

  enum leds_spi_cs_mode {
    LEDS_SPI_CS_MODE_DISABLED = -1,
    LEDS_SPI_CS_MODE_LOW      = 0,
    LEDS_SPI_CS_MODE_HIGH     = 1,
  };

  struct leds_spi_config {
    int host;
    uint16_t clock_pin;
    uint16_t data_pin;
  };

  extern struct leds_spi_config leds_spi_config;

  extern const struct config_enum leds_spi_host_enum[];
  extern const struct config_enum leds_spi_cs_mode_enum[];
  extern const struct config_enum leds_spi_clock_enum[];
#endif

#if CONFIG_LEDS_UART_ENABLED
  struct leds_uart_config {
    int port;
  };

  extern struct leds_uart_config leds_uart_config;

  extern const struct config_enum leds_uart_port_enum[];
#endif

#if CONFIG_LEDS_I2S_ENABLED
  #define I2S_CLOCK_20MHZ   (I2S_OUT_BASE_CLOCK / 8)
  #define I2S_CLOCK_10MHZ   (I2S_OUT_BASE_CLOCK / 16)
  #define I2S_CLOCK_5MHZ    (I2S_OUT_BASE_CLOCK / 32)
  #define I2S_CLOCK_4MHZ    (I2S_OUT_BASE_CLOCK / 40)
  #define I2S_CLOCK_2MHZ    (I2S_OUT_BASE_CLOCK / 80)
  #define I2S_CLOCK_1MHZ    (I2S_OUT_BASE_CLOCK / 160)
  #define I2S_CLOCK_500KHZ  (I2S_OUT_BASE_CLOCK / 320)
  #define I2S_CLOCK_200KHZ  (I2S_OUT_BASE_CLOCK / 800)
  #define I2S_CLOCK_100KHZ  (I2S_OUT_BASE_CLOCK / 1600)
  #define I2S_CLOCK_50KHZ   (I2S_OUT_BASE_CLOCK / 3200)

  #define I2S_CLOCK_DEFAULT (I2S_CLOCK_1MHZ)

  struct leds_i2s_config {
    int port;
  };

  extern struct leds_i2s_config leds_i2s_config;

  extern const struct config_enum leds_i2s_port_enum[];
  extern const struct config_enum leds_i2s_clock_enum[];
#endif

struct leds_config {
  bool enabled;

  int interface;
  int protocol;
  uint16_t count;
  uint16_t limit;

#if CONFIG_LEDS_SPI_ENABLED
  int spi_clock;
# if CONFIG_IDF_TARGET_ESP8266
  uint16_t spi_delay;
# else
  int spi_cs_mode;
  uint16_t spi_cs_io;
# endif
#endif

#if CONFIG_LEDS_I2S_ENABLED
  int i2s_clock;
# if LEDS_I2S_GPIO_PINS_ENABLED
  uint16_t i2s_clock_pin;
  uint16_t i2s_data_pin;
  uint16_t i2s_data_inv_pin;
# endif
#endif

#if CONFIG_LEDS_GPIO_ENABLED
  int gpio_mode;
  uint16_t gpio_pin[LEDS_GPIO_SIZE];
  unsigned gpio_count;
#endif

  bool test_enabled;

  bool artnet_enabled;
  uint16_t artnet_universe_start;
  uint16_t artnet_universe_count;
  uint16_t artnet_universe_step;
  uint16_t artnet_dmx_addr;
  uint16_t artnet_dmx_leds;
  uint16_t artnet_dmx_timeout;
  int artnet_leds_format;
  uint16_t artnet_leds_segment;
};

extern struct leds_config leds_configs[LEDS_COUNT];

extern const struct config_enum leds_interface_enum[];
extern const struct config_enum leds_protocol_enum[];
extern const struct config_enum leds_format_enum[];
extern const struct config_enum leds_test_mode_enum[];
extern const struct config_enum leds_color_parameter_enum[];

int config_leds(struct leds_state *state, const struct leds_config *config);

#if CONFIG_LEDS_GPIO_ENABLED
  int config_leds_gpio(struct leds_state *state, const struct leds_config *config, enum leds_interface interface, struct leds_interface_options_gpio *options);
#endif

#if CONFIG_LEDS_SPI_ENABLED
  int config_leds_spi(struct leds_state *state, const struct leds_config *config, struct leds_interface_spi_options *options);
#endif

#if CONFIG_LEDS_UART_ENABLED
  int config_leds_uart(struct leds_state *state, const struct leds_config *config, struct leds_interface_uart_options *options);
#endif

#if CONFIG_LEDS_I2S_ENABLED
  int config_leds_i2s(struct leds_state *state, const struct leds_config *config, struct leds_interface_i2s_options *options);
#endif
