#include "../leds.h"
#include "../leds_config.h"
#include "../leds_state.h"

#include <logging.h>


#if CONFIG_LEDS_SPI_ENABLED
// using custom spi_master driver
# include <spi_master.h>

#define LEDS_SPI_MODE 0 // varies by protocol
#define LEDS_SPI_PINS (SPI_PINS_CLK | SPI_PINS_MOSI)

#endif

const struct configtab leds_spi_configtab[] = {
  {},
};

#if CONFIG_LEDS_SPI_ENABLED
  struct spi_master *leds_spi_master;

  int init_leds_spi()
  {
    struct spi_options options = {
        .mode   = LEDS_SPI_MODE,
        .clock  = SPI_CLOCK_DEFAULT,
        .pins   = LEDS_SPI_PINS,
    };
    bool enabled = false;
    esp_err_t err;

    for (int i = 0; i < LEDS_COUNT; i++)
    {
      const struct leds_config *config = &leds_configs[i];

      if (!config->enabled) {
        continue;
      }

      if (config->interface != LEDS_INTERFACE_SPI) {
        continue;
      }

      enabled = true;

      if (config->spi_clock) {
        // match initial clock with output clock
        options.clock = config->spi_clock;
      }

      LOG_INFO("leds%d: spi configured", i + 1);
    }

    if (!enabled) {
      LOG_INFO("leds: spi not configured");
      return 0;
    }

    LOG_INFO("leds: spi mode=%02x clock=%u pins=%02x", options.mode, options.clock, options.pins);

    if ((err = spi_master_new(&leds_spi_master, options))) {
      LOG_ERROR("spi_master_new");
      return err;
    }

    return 0;
  }

  int config_leds_spi(struct leds_state *state, const struct leds_config *config, struct leds_options *options)
  {
    if (!leds_spi_master) {
      LOG_ERROR("leds%d: spi master not initialized", state->index + 1);
      return -1;
    }

    options->spi_master = leds_spi_master;
    options->spi_clock = config->spi_clock;

    if (config->spi_delay) {
      options->spi_mode_bits |= (config->spi_delay << SPI_MODE_MOSI_DELAY_SHIFT) & SPI_MODE_MOSI_DELAY_MASK;
    }

    LOG_INFO("leds%d: spi spi_mode_bits=%04x spi_clock=%u", state->index + 1,
      options->spi_mode_bits,
      options->spi_clock
    );

    return 0;
  }
#endif
