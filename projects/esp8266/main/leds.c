#include "leds.h"
#include "atx_psu.h"
#include "console.h"
#include "user_event.h"
#include "pin_mutex.h"

#include <spi_master.h>
#include <leds.h>

#include <logging.h>

#define LEDS_SPI_MODE 0 // varies by protocol
#define LEDS_SPI_PINS (SPI_PINS_CLK | SPI_PINS_MOSI)

#define LEDS_UART UART_1
#define LEDS_UART_RX_BUFFER_SIZE 0
#define LEDS_UART_TX_BUFFER_SIZE 512

#define LEDS_I2S_PORT I2S_PORT_0

struct uart *leds_uart;
struct spi_master *leds_spi_master;
struct i2s_out *leds_i2s_out;
struct gpio_out leds_gpio_out_uart, leds_gpio_out_spi, leds_gpio_out_i2s;
struct leds_state leds_states[LEDS_COUNT];

static int init_spi_master(const struct leds_config *configs)
{
  struct spi_options options = {
      .mode   = LEDS_SPI_MODE,
      .clock  = LEDS_SPI_CLOCK,
      .pins   = LEDS_SPI_PINS,
  };
  bool enabled = 0;
  int err;

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    const struct leds_config *config = &configs[i];
    enum leds_interface interface = config->interface ? config->interface : leds_interface_for_protocol(config->protocol);

    if (!config->enabled) {
      continue;
    }

    if (interface != LEDS_INTERFACE_SPI) {
      continue;
    }

    enabled = true;

    if (config->spi_clock) {
      // match initial clock with output clock
      options.clock = config->spi_clock;
    }

    switch (config->gpio_mode) {
      case LEDS_GPIO_OFF:
        break;

      case GPIO_OUT_HIGH:
        leds_gpio_out_spi.pins |= gpio_out_pin(config->gpio_pin);
        leds_gpio_out_spi.level = GPIO_OUT_HIGH; // XXX: per-pin output level?
        break;

      case GPIO_OUT_LOW:
        leds_gpio_out_spi.pins |= gpio_out_pin(config->gpio_pin);
        leds_gpio_out_spi.level = GPIO_OUT_LOW; // XXX: per-pin output level?
        break;
    }
  }

  LOG_INFO("enabled=%d mode=%02x clock=%u pins=%02x", enabled, options.mode, options.clock, options.pins);

  if (!enabled) {
    return 0;
  }

  LOG_INFO("gpio pins=%04x level=%d", leds_gpio_out_spi.pins, leds_gpio_out_spi.level);

  if ((err = spi_master_new(&leds_spi_master, options))) {
    LOG_ERROR("spi_master_new");
    return err;
  }

  if ((err = gpio_out_setup(&leds_gpio_out_spi))) {
    LOG_ERROR("gpio_out_setup");
    return err;
  }

  return 0;
}

static int init_uart(const struct leds_config *configs)
{
  bool enabled = 0;
  int err;

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    const struct leds_config *config = &configs[i];
    enum leds_interface interface = config->interface ? config->interface : leds_interface_for_protocol(config->protocol);

    if (!config->enabled) {
      continue;
    }

    if (interface != LEDS_INTERFACE_UART) {
      continue;
    }

    enabled = true;

    switch (config->gpio_mode) {
      case LEDS_GPIO_OFF:
        break;

      case GPIO_OUT_HIGH:
        leds_gpio_out_uart.pins |= gpio_out_pin(config->gpio_pin);
        leds_gpio_out_uart.level = GPIO_OUT_HIGH; // XXX: per-pin output level?
        break;

      case GPIO_OUT_LOW:
        leds_gpio_out_uart.pins |= gpio_out_pin(config->gpio_pin);
        leds_gpio_out_uart.level = GPIO_OUT_LOW; // XXX: per-pin output level?
        break;
    }
  }

  LOG_INFO("enabled=%d uart=%d tx_buffer_size=%u", enabled, LEDS_UART, LEDS_UART_TX_BUFFER_SIZE);

  if (!enabled) {
    return 0;
  }

  LOG_INFO("gpio pins=%04x level=%d", leds_gpio_out_uart.pins, leds_gpio_out_uart.level);

  if ((err = uart_new(&leds_uart, LEDS_UART, LEDS_UART_RX_BUFFER_SIZE, LEDS_UART_TX_BUFFER_SIZE))) {
    LOG_ERROR("uart_new");
    return err;
  }

  if ((err = gpio_out_setup(&leds_gpio_out_uart))) {
    LOG_ERROR("gpio_out_setup");
    return err;
  }

  return 0;
}

static int init_i2s_out(const struct leds_config *configs)
{
  size_t max_i2s_buffer_size = 0;
  bool enabled = 0;
  int err;

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    const struct leds_config *config = &configs[i];
    enum leds_interface interface = config->interface ? config->interface : leds_interface_for_protocol(config->protocol);

    size_t i2s_buffer_size;

    if (!config->enabled) {
      continue;
    }

    if (interface != LEDS_INTERFACE_I2S) {
      continue;
    }

    enabled = true;

    switch (config->gpio_mode) {
      case LEDS_GPIO_OFF:
        break;

      case GPIO_OUT_HIGH:
        leds_gpio_out_i2s.pins |= gpio_out_pin(config->gpio_pin);
        leds_gpio_out_i2s.level = GPIO_OUT_HIGH; // XXX: per-pin output level?
        break;

      case GPIO_OUT_LOW:
        leds_gpio_out_i2s.pins |= gpio_out_pin(config->gpio_pin);
        leds_gpio_out_i2s.level = GPIO_OUT_LOW; // XXX: per-pin output level?
        break;
    }

    // size TX buffer
    i2s_buffer_size = leds_i2s_buffer_for_protocol(config->protocol, config->count);

    if (i2s_buffer_size > max_i2s_buffer_size) {
      max_i2s_buffer_size = i2s_buffer_size;
    }
  }

  LOG_INFO("enabled=%d buffer_size=%u", enabled, max_i2s_buffer_size);

  if (!enabled) {
    return 0;
  }

  LOG_INFO("gpio pins=%04x level=%d", leds_gpio_out_i2s.pins, leds_gpio_out_i2s.level);

  if ((err = i2s_out_new(&leds_i2s_out, LEDS_I2S_PORT, max_i2s_buffer_size))) {
    LOG_ERROR("i2s_out_new");
    return err;
  }

  if ((err = gpio_out_setup(&leds_gpio_out_i2s))) {
    LOG_ERROR("gpio_out_setup");
    return err;
  }

  return 0;
}

static int init_leds_spi(struct leds_state *state, int index, const struct leds_config *config)
{
  struct leds_options options = {
      .interface  = LEDS_INTERFACE_SPI,
      .protocol   = config->protocol,
      .count      = config->count,

      .spi_master = leds_spi_master,
      .spi_clock  = config->spi_clock,

      .gpio_out   = &leds_gpio_out_spi,
  };
  int err;

  if (config->spi_delay) {
    options.spi_mode_bits |= (config->spi_delay << SPI_MODE_MOSI_DELAY_SHIFT) & SPI_MODE_MOSI_DELAY_MASK;
  }

  if (config->gpio_mode != LEDS_GPIO_OFF && gpio_out_pin(config->gpio_pin)) {
    options.gpio_out_pins = gpio_out_pin(config->gpio_pin);
  }

  LOG_INFO("leds%d: protocol=%u spi_mode_bits=%04x spi_clock=%u gpio_out_pins=%04x count=%u", index + 1, options.protocol, options.spi_mode_bits, options.spi_clock, options.gpio_out_pins, options.count);

  if ((err = leds_new(&state->leds, &options))) {
    LOG_ERROR("leds%d: leds_new", index + 1);
    return err;
  }

  return 0;
}

static int init_leds_uart(struct leds_state *state, int index, const struct leds_config *config)
{
  struct leds_options options = {
      .interface  = LEDS_INTERFACE_UART,
      .protocol   = config->protocol,
      .count      = config->count,

      .uart       = leds_uart,

      .gpio_out   = &leds_gpio_out_uart,
  };
  int err;

  if (config->gpio_mode != LEDS_GPIO_OFF && gpio_out_pin(config->gpio_pin)) {
    options.gpio_out_pins = gpio_out_pin(config->gpio_pin);
  }

  LOG_INFO("leds%d: protocol=%u gpio_out_pins=%04x count=%u", index + 1, options.protocol, options.gpio_out_pins, options.count);

  if ((err = leds_new(&state->leds, &options))) {
    LOG_ERROR("leds%d: leds_new", index + 1);
    return err;
  }

  return 0;
}

static int init_leds_i2s(struct leds_state *state, int index, const struct leds_config *config)
{
  struct leds_options options = {
      .interface  = LEDS_INTERFACE_I2S,
      .protocol   = config->protocol,
      .count      = config->count,

      .i2s_out        = leds_i2s_out,
      .i2s_pin_mutex  = pin_mutex[PIN_MUTEX_U0RXD], // shared between UART0_RXD and I2SO_DATA

      .gpio_out   = &leds_gpio_out_i2s,
  };
  int err;

  if (config->gpio_mode != LEDS_GPIO_OFF && gpio_out_pin(config->gpio_pin)) {
    options.gpio_out_pins = gpio_out_pin(config->gpio_pin);
  }

  LOG_INFO("leds%d: protocol=%u gpio_out_pins=%04x count=%u", index + 1, options.protocol, options.gpio_out_pins, options.count);

  if ((err = leds_new(&state->leds, &options))) {
    LOG_ERROR("leds%d: leds_new", index +'1');
    return err;
  }

  return 0;
}

static bool leds_enabled()
{
  for (int i = 0; i < LEDS_COUNT; i++)
  {
    const struct leds_config *config = &leds_configs[i];

    if (config->enabled) {
      return true;
    }
  }

  return false;
}

int init_leds()
{
  int err;

  if (!leds_enabled()) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = init_spi_master(leds_configs))) {
    LOG_ERROR("init_spi_master");
    return err;
  }

  if ((err = init_uart(leds_configs))) {
    LOG_ERROR("init_uart");
    return err;
  }

  if ((err = init_i2s_out(leds_configs))) {
    LOG_ERROR("init_i2s_out");
    return err;
  }

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    struct leds_state *state = &leds_states[i];
    const struct leds_config *config = &leds_configs[i];
    enum leds_interface interface = config->interface ? config->interface : leds_interface_for_protocol(config->protocol);

    state->config = config;

    if (!config->enabled) {
      continue;
    }

    switch (interface) {
      case LEDS_INTERFACE_SPI:
        if ((err = init_leds_spi(state, i, config))) {
          LOG_ERROR("leds%d: init_leds_spi", i + 1);
          return err;
        }
        break;

      case LEDS_INTERFACE_UART:
        if ((err = init_leds_uart(state, i, config))) {
          LOG_ERROR("leds%d: init_leds_uart", i + 1);
          return err;
        }
        break;

      case LEDS_INTERFACE_I2S:
        if ((err = init_leds_i2s(state, i, config))) {
          LOG_ERROR("leds%d: init_leds_i2s", i + 1);
          return err;
        }
        break;

      default:
        LOG_ERROR("unsupported protocol=%#x interface=%#x", config->protocol, interface);
        return -1;
    }

    if (config->test_enabled) {
      for (enum leds_test_mode mode = 0; mode <= TEST_MODE_END; mode++) {
        if ((err = test_leds(state, mode))) {
          LOG_ERROR("leds%d: test_leds", i + 1);
          return err;
        }
      }
    }

    if (config->artnet_enabled) {
      if ((err = init_leds_artnet(state, i, config))) {
        LOG_ERROR("leds%d: init_leds_artnet", i + 1);
        return err;
      }
    }
  }

  return 0;
}

static void update_leds_active(struct leds_state *state)
{
  bool active = false;

  state->active = leds_active(state->leds);

  LOG_DEBUG("active=%u", state->active);

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    struct leds_state *state = &leds_states[i];

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

int check_leds_interface(struct leds_state *state)
{
  switch (leds_interface(state->leds)) {
    case LEDS_INTERFACE_I2S:
      if (is_console_running()) {
        LOG_WARN("I2S out busy, console running on UART0");
        return 1;
      }
      return 0;

    default:
      return 0;
  }
}

int update_leds(struct leds_state *state)
{
  int err;

  update_leds_active(state);

  if ((err = check_leds_interface(state))) {
    return err;
  }

  user_activity(USER_ACTIVITY_LEDS);

  if ((err = leds_tx(state->leds))) {
    LOG_ERROR("leds_tx");
    return err;
  }

  return 0;
}

int test_leds(struct leds_state *state, enum leds_test_mode mode)
{
  int err;

  update_leds_active(state);

  if ((err = check_leds_interface(state))) {
    return err;
  }

  LOG_INFO("mode=%d", mode);

  user_activity(USER_ACTIVITY_LEDS);

  // animate
  TickType_t tick = xTaskGetTickCount();
  int ticks;

  for (unsigned frame = 0; ; frame++) {
    if ((ticks = leds_set_test(state->leds, mode, frame)) < 0) {
      LOG_ERROR("leds_set_test(%d, %u)", mode, frame);
      return ticks;
    }

    if ((err = leds_tx(state->leds))) {
      LOG_ERROR("leds_tx");
      return err;
    }

    if (ticks) {
      vTaskDelayUntil(&tick, ticks);
    } else {
      break;
    }
  }

  return 0;
}