#include "spi_leds.h"
#include "user_event.h"
#include "atx_psu.h"

#include <spi_master.h>
#include <spi_leds.h>

#include <logging.h>

#define SPI_LEDS_MODE 0 // varies by protocol
#define SPI_LEDS_PINS (SPI_PINS_CLK | SPI_PINS_MOSI)

struct spi_master *spi_master;
struct gpio_out spi_leds_gpio_out;
struct spi_leds_state spi_leds_states[SPI_LEDS_COUNT];

static int init_spi_master()
{
  struct spi_options options = {
      .mode   = SPI_LEDS_MODE,
      .clock  = SPI_LEDS_CLOCK,
      .pins   = SPI_LEDS_PINS,
  };
  int err;

  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    const struct spi_leds_config *config = &spi_leds_configs[i];

    if (!config->enabled) {
      continue;
    }

    if (config->spi_clock) {
      // match initial clock with output clock
      options.clock = config->spi_clock;
    }
  }

  LOG_INFO("mode=%02x clock=%u pins=%02x", options.mode, options.clock, options.pins);

  if ((err = spi_master_new(&spi_master, options))) {
    LOG_ERROR("spi_master_new");
    return err;
  }

  return 0;
}

static int init_gpio_out()
{
  enum gpio_out_pins gpio_out_pins = 0;
  enum gpio_out_level gpio_out_level = GPIO_OUT_LOW;
  int err;

  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    const struct spi_leds_config *config = &spi_leds_configs[i];

    if (!config->enabled) {
      continue;
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

  LOG_INFO("pins=%04x level=%d", gpio_out_pins, gpio_out_level);

  if ((err = gpio_out_init(&spi_leds_gpio_out, gpio_out_pins, gpio_out_level))) {
    LOG_ERROR("gpio_out_init");
    return err;
  }

  return 0;
}

static int init_spi_leds_state(struct spi_leds_state *state, int index, const struct spi_leds_config *config)
{
  struct spi_leds_options options = {
      .interface  = SPI_LEDS_INTERFACE_SPI,
      .protocol   = config->protocol,
      .count      = config->count,

      .spi_master = spi_master,
      .spi_clock  = config->spi_clock,

      .gpio_out   = &spi_leds_gpio_out,
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

  if ((err = init_gpio_out(spi_leds_configs))) {
    LOG_ERROR("init_gpio_out");
    return err;
  }

  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    struct spi_leds_state *state = &spi_leds_states[i];
    const struct spi_leds_config *config = &spi_leds_configs[i];

    if (!config->enabled) {
      continue;
    }

    if ((err = init_spi_leds_state(state, i, config))) {
      LOG_ERROR("spi-leds%d: config_spi_leds", i);
      return err;
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

  for (enum spi_leds_test_mode mode = 0; mode < TEST_MODE_MAX; mode++) {
    user_activity(USER_ACTIVITY_SPI_LEDS);

    if ((err = spi_leds_test(state->spi_leds, mode))) {
      LOG_ERROR("spi_leds_test");
      return err;
    }
  }

  return 0;
}
