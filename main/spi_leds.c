#include "spi_leds.h"
#include "user_event.h"
#include "atx_psu.h"

#include <spi_master.h>
#include <spi_leds.h>

#include <logging.h>

#define SPI_LEDS_MODE 0 // varies by protocol
#define SPI_LEDS_PINS (SPI_PINS_CLK | SPI_PINS_MOSI)

#define UART1_BAUD_RATE UART1_BAUD_250000
#define UART1_DATA_BITS UART1_DATA_BITS_8
#define UART1_PARITY_BITS UART1_PARTIY_DISABLE
#define UART1_STOP_BITS UART1_STOP_BITS_1
#define UART1_TX_BUFFER_SIZE 512

struct uart1 *spi_leds_uart1;
struct spi_master *spi_leds_spi_master;
struct gpio_out spi_leds_gpio_out_uart, spi_leds_gpio_out_spi;
struct spi_leds_state spi_leds_states[SPI_LEDS_COUNT];

static int init_spi_master(const struct spi_leds_config *configs)
{
  enum gpio_out_pins gpio_out_pins = 0;
  enum gpio_out_level gpio_out_level = GPIO_OUT_LOW;
  struct spi_options options = {
      .mode   = SPI_LEDS_MODE,
      .clock  = SPI_LEDS_CLOCK,
      .pins   = SPI_LEDS_PINS,
  };
  bool enabled = 0;
  int err;

  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    const struct spi_leds_config *config = &configs[i];

    if (!config->enabled) {
      continue;
    }

    if (spi_leds_interface_for_protocol(config->protocol) != SPI_LEDS_INTERFACE_SPI) {
      continue;
    }

    enabled = true;

    if (config->spi_clock) {
      // match initial clock with output clock
      options.clock = config->spi_clock;
    }

    switch (config->gpio_mode) {
      case SPI_LEDS_GPIO_OFF:
        break;

      case GPIO_OUT_HIGH:
        gpio_out_pins |= gpio_out_pin(config->gpio_pin);
        gpio_out_level = GPIO_OUT_HIGH; // XXX: per-pin output level?
        break;

      case GPIO_OUT_LOW:
        gpio_out_pins |= gpio_out_pin(config->gpio_pin);
        gpio_out_level = GPIO_OUT_LOW; // XXX: per-pin output level?
        break;
    }
  }

  if (!enabled) {
    return 0;
  }

  LOG_INFO("enabled=%d mode=%02x clock=%u pins=%02x", enabled, options.mode, options.clock, options.pins);
  LOG_INFO("gpio pins=%04x level=%d", gpio_out_pins, gpio_out_level);

  if ((err = spi_master_new(&spi_leds_spi_master, options))) {
    LOG_ERROR("spi_master_new");
    return err;
  }

  if ((err = gpio_out_init(&spi_leds_gpio_out_spi, gpio_out_pins, gpio_out_level))) {
    LOG_ERROR("gpio_out_init");
    return err;
  }

  return 0;
}

static int init_uart1(const struct spi_leds_config *configs)
{
  enum gpio_out_pins gpio_out_pins = 0;
  enum gpio_out_level gpio_out_level = GPIO_OUT_LOW;
  struct uart1_options options = {
      .clock_div    = UART1_BAUD_RATE,
      .data_bits    = UART1_DATA_BITS,
      .parity_bits  = UART1_PARITY_BITS,
      .stop_bits    = UART1_STOP_BITS_1,
  };
  bool enabled = 0;
  int err;

  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    const struct spi_leds_config *config = &configs[i];

    if (!config->enabled) {
      continue;
    }

    if (spi_leds_interface_for_protocol(config->protocol) != SPI_LEDS_INTERFACE_UART) {
      continue;
    }

    enabled = true;

    switch (config->gpio_mode) {
      case SPI_LEDS_GPIO_OFF:
        break;

      case GPIO_OUT_HIGH:
        gpio_out_pins |= gpio_out_pin(config->gpio_pin);
        gpio_out_level = GPIO_OUT_HIGH; // XXX: per-pin output level?
        break;

      case GPIO_OUT_LOW:
        gpio_out_pins |= gpio_out_pin(config->gpio_pin);
        gpio_out_level = GPIO_OUT_LOW; // XXX: per-pin output level?
        break;
    }
  }

  if (!enabled) {
    return 0;
  }

  LOG_INFO("enabled=%d", enabled);
  LOG_INFO("gpio pins=%04x level=%d", gpio_out_pins, gpio_out_level);

  if ((err = uart1_new(&spi_leds_uart1, options, UART1_TX_BUFFER_SIZE))) {
    LOG_ERROR("uart1_new");
    return err;
  }

  if ((err = gpio_out_init(&spi_leds_gpio_out_uart, gpio_out_pins, gpio_out_level))) {
    LOG_ERROR("gpio_out_init");
    return err;
  }

  return 0;
}

static int init_spi_leds_spi(struct spi_leds_state *state, int index, const struct spi_leds_config *config)
{
  struct spi_leds_options options = {
      .interface  = SPI_LEDS_INTERFACE_SPI,
      .protocol   = config->protocol,
      .count      = config->count,

      .spi_master = spi_leds_spi_master,
      .spi_clock  = config->spi_clock,

      .gpio_out   = &spi_leds_gpio_out_spi,
  };
  int err;

  if (config->delay) {
    options.spi_mode_bits |= (config->delay << SPI_MODE_MOSI_DELAY_SHIFT) & SPI_MODE_MOSI_DELAY_MASK;
  }

  if (config->gpio_mode != SPI_LEDS_GPIO_OFF && gpio_out_pin(config->gpio_pin)) {
    options.gpio_out_pins = gpio_out_pin(config->gpio_pin);
  }

  LOG_INFO("spi-leds%d: protocol=%u spi_mode_bits=%04x spi_clock=%u gpio_out_pins=%04x count=%u", index, options.protocol, options.spi_mode_bits, options.spi_clock, options.gpio_out_pins, options.count);

  if ((err = spi_leds_new(&state->spi_leds, &options))) {
    LOG_ERROR("spi-leds%d: spi_leds_new", index);
    return err;
  }

  return 0;
}

static int init_spi_leds_uart(struct spi_leds_state *state, int index, const struct spi_leds_config *config)
{
  struct spi_leds_options options = {
      .interface  = SPI_LEDS_INTERFACE_UART,
      .protocol   = config->protocol,
      .count      = config->count,

      .uart1      = spi_leds_uart1,

      .gpio_out   = &spi_leds_gpio_out_uart,
  };
  int err;

  if (config->gpio_mode != SPI_LEDS_GPIO_OFF && gpio_out_pin(config->gpio_pin)) {
    options.gpio_out_pins = gpio_out_pin(config->gpio_pin);
  }

  LOG_INFO("spi-leds%d: protocol=%u gpio_out_pins=%04x count=%u", index, options.protocol, options.gpio_out_pins, options.count);

  if ((err = spi_leds_new(&state->spi_leds, &options))) {
    LOG_ERROR("spi-leds%d: spi_leds_new", index);
    return err;
  }

  return 0;
}

static bool spi_leds_enabled()
{
  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    const struct spi_leds_config *config = &spi_leds_configs[i];

    if (config->enabled) {
      return true;
    }
  }

  return false;
}

int init_spi_leds()
{
  int err;

  if (!spi_leds_enabled()) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = init_spi_master(spi_leds_configs))) {
    LOG_ERROR("init_spi_master");
    return err;
  }

  if ((err = init_uart1(spi_leds_configs))) {
    LOG_ERROR("init_uart1");
    return err;
  }

  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    struct spi_leds_state *state = &spi_leds_states[i];
    const struct spi_leds_config *config = &spi_leds_configs[i];
    enum spi_leds_interface interface = spi_leds_interface_for_protocol(config->protocol);

    state->config = config;

    if (!config->enabled) {
      continue;
    }

    switch (interface) {
      case SPI_LEDS_INTERFACE_SPI:
        if ((err = init_spi_leds_spi(state, i, config))) {
          LOG_ERROR("spi-leds%d: init_spi_leds_spi", i);
          return err;
        }
        break;

      case SPI_LEDS_INTERFACE_UART:
        if ((err = init_spi_leds_uart(state, i, config))) {
          LOG_ERROR("spi-leds%d: init_spi_leds_uart", i);
          return err;
        }
        break;

      default:
        LOG_ERROR("unsupported protocol=%#x interface=%#x", config->protocol, interface);
        return -1;
    }

    if (config->test_enabled) {
      if ((err = test_spi_leds(state))) {
        LOG_ERROR("spi-leds%d: test_spi_leds", i);
        return err;
      }
    }

    if (config->artnet_enabled) {
      if ((err = init_spi_leds_artnet(state, i, config))) {
        LOG_ERROR("spi-leds%d: init_spi_leds_artnet", i);
        return err;
      }
    }
  }

  return 0;
}

static void update_spi_leds_active()
{
  bool active = false;

  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    struct spi_leds_state *state = &spi_leds_states[i];

    if (state->active) {
      active = true;
      break;
    }
  }

  if (active) {
    activate_atx_psu(ATX_PSU_BIT_SPI_LED);
  } else {
    deactivate_atx_psu(ATX_PSU_BIT_SPI_LED);
  }
}

int update_spi_leds(struct spi_leds_state *state)
{
  int err;

  state->active = spi_leds_active(state->spi_leds);

  LOG_DEBUG("active=%u", state->active);

  update_spi_leds_active();
  user_activity(USER_ACTIVITY_SPI_LEDS);

  if ((err = spi_leds_tx(state->spi_leds))) {
    LOG_ERROR("spi_leds_tx");
    return err;
  }

  return 0;
}

int test_spi_leds(struct spi_leds_state *state)
{
  int err;

  state->active = true;

  LOG_DEBUG("active=%u", state->active);

  update_spi_leds_active();

  for (enum spi_leds_test_mode mode = 0; mode <= TEST_MODE_END; mode++) {
    user_activity(USER_ACTIVITY_SPI_LEDS);

    if ((err = spi_leds_test(state->spi_leds, mode))) {
      LOG_ERROR("spi_leds_test");
      return err;
    }
  }

  return 0;
}
