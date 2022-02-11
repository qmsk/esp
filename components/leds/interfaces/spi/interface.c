#include "../spi.h"
#include "../../leds.h"

#include <logging.h>

#include <esp_err.h>

#if CONFIG_IDF_TARGET_ESP8266
  int leds_interface_spi_init(struct leds_interface_spi *interface, const struct leds_options *options, void **bufp, size_t buf_size, int spi_mode)
  {
    void *buf;

    interface->spi_master = options->spi_master;
    interface->options = (struct spi_write_options) {
      .mode   = options->spi_mode_bits | spi_mode | SPI_MODE_SET,
      .clock  = options->spi_clock,
    };

    if (!(buf = malloc(buf_size))) {
      LOG_ERROR("malloc");
      return -1;
    } else {
      *bufp = buf;
    }

    return 0;
  }

  int leds_interface_spi_tx(struct leds_interface_spi *interface, const struct leds_options *options, void *buf, size_t size)
  {
    uint8_t *ptr = buf;
    unsigned len = size;
    int ret, err;

    if ((err = spi_master_open(interface->spi_master, interface->options))) {
      LOG_ERROR("spi_master_open");
      return err;
    }

  #if CONFIG_LEDS_GPIO_ENABLED
    if (options->gpio_out) {
      gpio_out_set(options->gpio_out, options->gpio_out_pins);
    }
  #endif

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

  error:
  #if CONFIG_LEDS_GPIO_ENABLED
    if (options->gpio_out) {
      gpio_out_clear(options->gpio_out);
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

  int leds_interface_spi_init(struct leds_interface_spi *interface, const struct leds_options *options, void **bufp, size_t buf_size, int spi_mode)
  {
    spi_device_interface_config_t device_config = {
      .command_bits   = 0,
      .address_bits   = 0,
      .dummy_bits     = 0,
      .mode           = spi_mode,
      .clock_speed_hz = options->spi_clock,
      .spics_io_num   = options->spi_cs_io,
      .queue_size     = 1,
    };
    void *buf;
    esp_err_t err;

    if (options->spi_cs_high) {
      device_config.flags |= SPI_DEVICE_POSITIVE_CS;
    }

    if ((err = spi_bus_add_device(options->spi_host, &device_config, &interface->device))) {
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

  int leds_interface_spi_tx(struct leds_interface_spi *interface, const struct leds_options *options, void *buf, size_t size)
  {
    spi_transaction_t transaction = {
      .length = size * 8, // transaction length is in bits
      .tx_buffer = buf,
    };
    esp_err_t err;

    // TODO: additional gpio_out support, if spi_cs is not enough?
    if ((err = spi_device_transmit(interface->device, &transaction))) {
      LOG_ERROR("spi_device_transmit");
      return -1;
    }

    return 0;
  }
#endif
