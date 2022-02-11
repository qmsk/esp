#pragma once

#include <config.h>
#include <leds.h>

#include <stdbool.h>

struct leds_state *state;

struct leds_config {
  bool enabled;

  int interface;
  int protocol;
  uint16_t count;

  int spi_clock;
  uint16_t spi_delay;

  int uart_port;

  int gpio_mode;
  uint16_t gpio_pin;

  bool test_enabled;

  bool artnet_enabled;
  uint16_t artnet_universe_start;
  uint16_t artnet_universe_count;
  uint16_t artnet_universe_step;
  uint16_t artnet_dmx_addr;
  uint16_t artnet_dmx_leds;
  int artnet_leds_format;
  uint16_t artnet_leds_segment;
};

extern struct leds_config leds_configs[LEDS_COUNT];

extern const struct config_enum leds_interface_enum[];
extern const struct config_enum leds_protocol_enum[];
#if CONFIG_LEDS_UART_ENABLED
  extern const struct config_enum leds_uart_port_enum[];
#endif
extern const struct config_enum leds_spi_clock_enum[];
extern const struct config_enum leds_gpio_mode_enum[];
extern const struct config_enum leds_format_enum[];
extern const struct config_enum leds_test_mode_enum[];
extern const struct config_enum leds_color_parameter_enum[];

int config_leds(struct leds_state *state, const struct leds_config *config);
