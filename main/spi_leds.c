#include "spi_leds.h"
#include "activity_led.h"
#include "atx_psu.h"

#include <spi_master.h>
#include <spi_leds.h>

#include <logging.h>

#define SPI_LEDS_MODE (SPI_MODE_0) // varies by protocol
#define SPI_LEDS_CLOCK (SPI_CLOCK_1MHZ)
#define SPI_LEDS_PINS (SPI_PINS_CLK | SPI_PINS_MOSI)

struct spi_master *spi_master;
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

    if (config->gpio_mode != SPI_LEDS_GPIO_OFF && spi_gpio_from_pin(config->gpio_pin)) {
      options.gpio |= spi_gpio_from_pin(config->gpio_pin) | (config->gpio_mode);
    }
  }

  LOG_INFO("mode=%02x clock=%u pins=%02x gpio=%04x", options.mode, options.clock, options.pins, options.gpio);

  if ((err = spi_master_new(&spi_master, options))) {
    LOG_ERROR("spi_master_new");
    return err;
  }

  return 0;
}

static int init_spi_leds_state(struct spi_leds_state *state, int index, const struct spi_leds_config *config)
{
  struct spi_leds_options options = {
      .protocol = config->protocol,
      .clock    = config->spi_clock,
      .count    = config->count,
  };
  int err;

  if (config->gpio_mode != SPI_LEDS_GPIO_OFF && spi_gpio_from_pin(config->gpio_pin)) {
    options.gpio = spi_gpio_from_pin(config->gpio_pin) | (config->gpio_mode);
  }

  LOG_INFO("spi-leds%d: protocol=%u clock=%u gpio=%04x count=%u", index, options.protocol, options.clock, options.gpio, options.count);

  if ((err = spi_leds_new(&state->spi_leds, spi_master, options))) {
    LOG_ERROR("spi-leds%d: spi_leds_new", index);
    return err;
  }

  if (config->artnet_enabled) {
    if ((err = init_spi_leds_artnet(state, index, config))) {
      LOG_ERROR("spi-leds%d: init_spi_leds_artnet", index);
      return err;
    }
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
    LOG_ERROR("config_spi_master");
    return err;
  }

  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    const struct spi_leds_config *config = &spi_leds_configs[i];

    if (!config->enabled) {
      continue;
    }

    if ((err = init_spi_leds_state(&spi_leds_states[i], i, config))) {
      LOG_ERROR("spi-leds%d: config_spi_leds", i);
      return err;
    }
  }

  return 0;
}

static void update_spi_leds_active()
{
  bool active;

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
  activity_led_event();

  if ((err = spi_leds_tx(state->spi_leds))) {
    LOG_ERROR("spi_leds_tx");
    return err;
  }

  return 0;
}
