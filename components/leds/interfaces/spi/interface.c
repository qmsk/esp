#include "../spi.h"
#include "../../leds.h"
#include "../../stats.h"

#include <logging.h>

#include <esp_err.h>

#if CONFIG_IDF_TARGET_ESP8266
  int leds_interface_spi_init(struct leds_interface_spi *interface, const struct leds_interface_spi_options *options, void **bufp, size_t buf_size, int spi_mode)
  {
    void *buf;

    interface->spi_master = options->spi_master;
    interface->options = (struct spi_write_options) {
      .mode   = options->mode_bits | spi_mode | SPI_MODE_SET,
      .clock  = options->clock,
    };

    if (!(buf = malloc(buf_size))) {
      LOG_ERROR("malloc");
      return -1;
    } else {
      *bufp = buf;
    }

    return 0;
  }

  int leds_interface_spi_tx(struct leds_interface_spi *interface, const struct leds_interface_spi_options *options, void *buf, size_t size)
  {
    struct leds_interface_spi_stats *stats = &leds_interface_stats.spi;
    uint8_t *ptr = buf;
    unsigned len = size;
    int ret, err;

    WITH_STATS_TIMER(&stats->open) {
      if ((err = spi_master_open(interface->spi_master, interface->options))) {
        LOG_ERROR("spi_master_open");
        return err;
      }
    }

  #if CONFIG_LEDS_GPIO_ENABLED
    if (options->gpio.gpio_options) {
      gpio_out_set(options->gpio.gpio_options, options->gpio.gpio_out_pins);
    }
  #endif

    WITH_STATS_TIMER(&stats->tx) {
      while (len) {
        if ((ret = spi_master_write(interface->spi_master, ptr, len)) < 0) {
          LOG_ERROR("spi_master_write");
          err = ret;
          goto error;
        }

        LOG_DEBUG("spi_master=%p write %p @ %u -> %d", interface->spi_master, ptr, len, ret);

        ptr += ret;
        len -= ret;
      }

      // wait for write TX to complete before clearing gpio to release bus
      if ((err = spi_master_flush(interface->spi_master)) < 0) {
        LOG_ERROR("spi_master_flush");
        goto error;
      }
    }

  error:
  #if CONFIG_LEDS_GPIO_ENABLED
    if (options->gpio.gpio_options) {
      gpio_out_clear(options->gpio.gpio_options);
    }
  #endif

    if ((err = spi_master_close(interface->spi_master))) {
      LOG_ERROR("spi_master_close");
      return err;
    }

    return err;
  }
#else
# include <esp_heap_caps.h>
  // give CS pin a couple clock cycles to stabilize
  #define SPI_DEVICE_CS_ENA_PRETRANS_DEFAULT 4
  #define SPI_DEVICE_CS_ENA_POSTTRANS_DEFAULT 2

  int leds_interface_spi_init(struct leds_interface_spi *interface, const struct leds_interface_spi_options *options, void **bufp, size_t buf_size, int spi_mode)
  {
    spi_device_interface_config_t device_config = {
      .command_bits     = 0,
      .address_bits     = 0,
      .dummy_bits       = 0,
      .mode             = spi_mode,
      .cs_ena_pretrans  = SPI_DEVICE_CS_ENA_PRETRANS_DEFAULT,
      .cs_ena_posttrans = SPI_DEVICE_CS_ENA_POSTTRANS_DEFAULT,
      .clock_speed_hz   = options->clock,
      .spics_io_num     = options->cs_io,
      .flags            = (
        // required for cs_ena_pretrans, irrelevant as we skip the MISO phase
        SPI_DEVICE_HALFDUPLEX
      ),
      .queue_size       = 1,
    };
    void *buf;
    esp_err_t err;

    if (options->cs_high) {
      device_config.flags |= SPI_DEVICE_POSITIVE_CS;
    }

    if ((err = spi_bus_add_device(options->host, &device_config, &interface->device))) {
      LOG_ERROR("spi_bus_add_device");
      return -1;
    }

    // use DMA-capable memory for the TX buffer
    if (!(buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA))) {
      LOG_ERROR("heap_caps_malloc(%u)", buf_size);
      return -1;
    } else {
      *bufp = buf;
    }

    return 0;
  }

  int leds_interface_spi_tx(struct leds_interface_spi *interface, const struct leds_interface_spi_options *options, void *buf, size_t size)
  {
    struct leds_interface_spi_stats *stats = &leds_interface_stats.spi;
    spi_transaction_t transaction = {
      .length = size * 8, // transaction length is in bits
      .tx_buffer = buf,
    };
    int ret = 0;
    esp_err_t err;

    WITH_STATS_TIMER(&stats->open) {
      if ((err = spi_device_acquire_bus(interface->device, portMAX_DELAY))) {
        LOG_ERROR("spi_device_acquire_bus: %s", esp_err_to_name(err));
        return -1;
      }
    }

  #if CONFIG_LEDS_GPIO_ENABLED
    if (options->gpio.gpio_options) {
      gpio_out_set(options->gpio.gpio_options, options->gpio.gpio_out_pins);
    }
  #endif

    WITH_STATS_TIMER(&stats->tx) {
      if ((ret = spi_device_transmit(interface->device, &transaction))) {
        LOG_ERROR("spi_device_transmit");
        goto error;
      }
    }

error:
  #if CONFIG_LEDS_GPIO_ENABLED
    if (options->gpio.gpio_options) {
      gpio_out_clear(options->gpio.gpio_options);
    }
  #endif

    spi_device_release_bus(interface->device);

    return ret;
  }
#endif
