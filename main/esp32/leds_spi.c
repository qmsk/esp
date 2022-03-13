#include "../leds.h"
#include "../leds_config.h"
#include "../leds_state.h"

#if CONFIG_LEDS_SPI_ENABLED
// using esp-idf spi_master driver
# include <driver/spi_master.h>
# include <soc/spi_pins.h>
#endif

#include <logging.h>

#if CONFIG_LEDS_SPI_ENABLED
  struct leds_spi_config leds_spi_config = {

  };

  const struct config_enum leds_spi_host_enum[] = {
    { "",       -1        },
  #if SOC_SPI_PERIPH_NUM >= 1
    { "SPI1",   SPI1_HOST },
  #endif
  #if SOC_SPI_PERIPH_NUM >= 2
    { "SPI2",   SPI2_HOST },
  #endif
  #if SOC_SPI_PERIPH_NUM >= 3
    { "SPI3",   SPI3_HOST },
  #endif
    {}
  };

  #if SOC_SPI_PERIPH_NUM >= 3
    #define LEDS_SPI_CONFIG_HOST_DEFAULT_VALUE SPI3_HOST
    #define LEDS_SPI_CONFIG_CLOCK_PIN_DEFAULT_VALUE SPI3_IOMUX_PIN_NUM_CLK
    #define LEDS_SPI_CONFIG_DATA_PIN_DEFAULT_VALUE SPI3_IOMUX_PIN_NUM_MOSI
  #elif SOC_SPI_PERIPH_NUM >= 2
    #define LEDS_SPI_CONFIG_HOST_DEFAULT_VALUE SPI2_HOST
    #define LEDS_SPI_CONFIG_CLOCK_PIN_DEFAULT_VALUE SPI2_IOMUX_PIN_NUM_CLK
    #define LEDS_SPI_CONFIG_DATA_PIN_DEFAULT_VALUE SPI2_IOMUX_PIN_NUM_MOSI
  #elif SOC_SPI_PERIPH_NUM >= 1
    #define LEDS_SPI_CONFIG_HOST_DEFAULT_VALUE SPI1_HOST
    #define LEDS_SPI_CONFIG_CLOCK_PIN_DEFAULT_VALUE SPI_IOMUX_PIN_NUM_CLK
    #define LEDS_SPI_CONFIG_DATA_PIN_DEFAULT_VALUE SPI_IOMUX_PIN_NUM_MOSI
  #else
    #define LEDS_SPI_CONFIG_HOST_DEFAULT_VALUE -1
  #endif

#endif

const struct configtab leds_spi_configtab[] = {
#if CONFIG_LEDS_SPI_ENABLED
  { CONFIG_TYPE_ENUM, "host",
    .description = "Select host peripherial for SPI interface.",
    .enum_type = { .value = &leds_spi_config.host, .values = leds_spi_host_enum, .default_value = LEDS_SPI_CONFIG_HOST_DEFAULT_VALUE },
  },
  { CONFIG_TYPE_UINT16, "clock_pin",
    .description = "Output SPI Clock to GPIO pin.",
    .uint16_type = { .value = &leds_spi_config.clock_pin, .max = (GPIO_NUM_MAX - 1), .default_value = LEDS_SPI_CONFIG_CLOCK_PIN_DEFAULT_VALUE },
  },
  { CONFIG_TYPE_UINT16, "data_pin",
    .description = "Output SPI MOSI to GPIO pin.",
    .uint16_type = { .value = &leds_spi_config.data_pin, .max = (GPIO_NUM_MAX - 1), .default_value = LEDS_SPI_CONFIG_DATA_PIN_DEFAULT_VALUE },
  },
#endif
  {},
};

#if CONFIG_LEDS_SPI_ENABLED
  spi_host_device_t leds_spi_host = -1;

  int init_leds_spi()
  {
    const struct leds_spi_config *spi_config = &leds_spi_config;
    spi_bus_config_t bus_config = {
      .mosi_io_num = spi_config->data_pin,
      .miso_io_num = -1,
      .sclk_io_num = spi_config->clock_pin,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
    };
    bool enabled = false;
    esp_err_t err;

    if (spi_config->host < 0) {
      LOG_INFO("leds: spi host disabled");
      return 0;
    }

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

      // update maximum trasnfer size
      size_t data_size = leds_spi_buffer_for_protocol(config->protocol, config->count);

      LOG_INFO("leds%d: spi data buffer size=%u", i + 1, data_size);

      if (data_size > bus_config.max_transfer_sz) {
        bus_config.max_transfer_sz = data_size;
      }
    }

    if (!enabled) {
      LOG_INFO("leds: spi not configured");
      return 0;
    }

    LOG_INFO("leds: spi host=%d -> mosi_io_num=%d sclk_io_num=%d max_transfer_sz=%d", spi_config->host,
      bus_config.mosi_io_num,
      bus_config.sclk_io_num,
      bus_config.max_transfer_sz
    );

    if ((err = spi_bus_initialize(spi_config->host, &bus_config, SPI_DMA_CH_AUTO))) {
      LOG_ERROR("spi_bus_initialize(%d): %s", spi_config->host, esp_err_to_name(err));
      return -1;
    } else {
      leds_spi_host = spi_config->host;
    }

    return 0;
  }

  int config_leds_spi(struct leds_state *state, const struct leds_config *config, struct leds_interface_spi_options *options)
  {
    if (leds_spi_host == -1) {
      LOG_ERROR("leds%d: spi host not initialized", state->index + 1);
      return -1;
    }

    options->host = leds_spi_host;
    options->clock = config->spi_clock;

    switch ((enum leds_spi_cs_mode)(config->spi_cs_mode)) {
      case LEDS_SPI_CS_MODE_DISABLED:
        options->cs_io = -1; // disabled
        options->cs_high = false;
        break;

      case LEDS_SPI_CS_MODE_LOW:
        options->cs_io = config->spi_cs_io;
        options->cs_high = false;
        break;

      case LEDS_SPI_CS_MODE_HIGH:
        options->cs_io = config->spi_cs_io;
        options->cs_high = true;
        break;
    }

    LOG_INFO("leds%d: spi host=%d clock=%d cs_io=%d cs_high=%d", state->index + 1,
      options->host,
      options->clock,
      options->cs_io,
      options->cs_high
    );

    return 0;
  }
#endif
