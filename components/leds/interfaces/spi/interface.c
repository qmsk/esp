#include "../spi.h"
#include "../../leds.h"
#include "../../stats.h"
#include "../../gpio.h"

#include <logging.h>

#include <esp_err.h>

#if CONFIG_IDF_TARGET_ESP8266
  static enum spi_mode leds_interface_spi_master_mode(enum leds_interface_spi_mode mode)
  {
    switch (mode) {
      case LEDS_INTERFACE_SPI_MODE0_32BIT:
        return SPI_MODE_0;

      case LEDS_INTERFACE_SPI_MODE1_32BIT:
        return SPI_MODE_1;

      case LEDS_INTERFACE_SPI_MODE2_32BIT:
        return SPI_MODE_2;

      case LEDS_INTERFACE_SPI_MODE3_32BIT:
        return SPI_MODE_3;

      default:
        LOG_FATAL("invalid mode=%d", mode);

    }
  }

  static int leds_interface_spi_master_init(struct leds_interface_spi *interface, const struct leds_interface_spi_options *options)
  {
    interface->spi_master = options->spi_master;
    interface->spi_write_options = (struct spi_write_options) {
      .mode   = options->mode_bits | leds_interface_spi_master_mode(interface->mode) | SPI_MODE_SET,
      .clock  = options->clock,
    };

    if (!(interface->buf.p = malloc(interface->buf_size))) {
      LOG_ERROR("malloc");
      return -1;
    }

    return 0;
  }

  static int leds_interface_spi_master_open(struct leds_interface_spi *interface)
  {
    return spi_master_open(interface->spi_master, interface->spi_write_options);
  }

  static int leds_interface_spi_master_write(struct leds_interface_spi *interface, size_t len)
  {
    uint8_t *buf = interface->buf.u8;
    int ret;

    while (len) {
      if ((ret = spi_master_write(interface->spi_master, buf, len)) < 0) {
        LOG_ERROR("spi_master_write");
        return ret;
      }

      LOG_DEBUG("spi_master=%p write %p @ %u -> %d", interface->spi_master, buf, len, ret);

      buf += ret;
      len -= ret;
    }

    return 0;
  }

  static int leds_interface_spi_master_flush(struct leds_interface_spi *interface)
  {
    return spi_master_flush(interface->spi_master);
  }

  static int leds_interface_spi_master_close(struct leds_interface_spi *interface)
  {
    return spi_master_close(interface->spi_master);
  }

#else
  #include <esp_heap_caps.h>

  // give CS pin a couple clock cycles to stabilize
  #define SPI_DEVICE_CS_ENA_PRETRANS_DEFAULT 4
  #define SPI_DEVICE_CS_ENA_POSTTRANS_DEFAULT 2

  static uint8_t leds_interface_spi_master_mode(enum leds_interface_spi_mode mode)
  {
    switch (mode) {
      case LEDS_INTERFACE_SPI_MODE0_32BIT:
        return 0;

      case LEDS_INTERFACE_SPI_MODE1_32BIT:
        return 1;

      case LEDS_INTERFACE_SPI_MODE2_32BIT:
        return 2;

      case LEDS_INTERFACE_SPI_MODE3_32BIT:
        return 3;

      default:
        LOG_FATAL("invalid mode=%d", mode);

    }
  }

  static int leds_interface_spi_master_init(struct leds_interface_spi *interface, const struct leds_interface_spi_options *options)
  {
    spi_device_interface_config_t device_config = {
      .command_bits     = 0,
      .address_bits     = 0,
      .dummy_bits       = 0,
      .mode             = leds_interface_spi_master_mode(interface->mode),
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
    esp_err_t err;

    if (options->cs_high) {
      device_config.flags |= SPI_DEVICE_POSITIVE_CS;
    }

    if ((err = spi_bus_add_device(options->host, &device_config, &interface->device))) {
      LOG_ERROR("spi_bus_add_device");
      return -1;
    }

    // use DMA-capable memory for the TX buffer
    if (!(interface->buf.p = heap_caps_malloc(interface->buf_size, MALLOC_CAP_DMA))) {
      LOG_ERROR("heap_caps_malloc(%u)", interface->buf_size);
      return -1;
    }

    return 0;
  }

  static int leds_interface_spi_master_open(struct leds_interface_spi *interface)
  {
    return spi_device_acquire_bus(interface->device, portMAX_DELAY);
  }

  static int leds_interface_spi_master_write(struct leds_interface_spi *interface, size_t len)
  {
    spi_transaction_t transaction = {
      .length    = len * 8, // transaction length is in bits
      .tx_buffer = interface->buf.p,
    };

    return spi_device_transmit(interface->device, &transaction);
  }

  static int leds_interface_spi_master_flush(struct leds_interface_spi *interface)
  {
    return 0; // no-op, transaction waits for copmlete
  }

  static int leds_interface_spi_master_close(struct leds_interface_spi *interface)
  {
    spi_device_release_bus(interface->device);

    return 0;
  }

#endif

int leds_interface_spi_init(struct leds_interface_spi *interface, const struct leds_interface_spi_options *options, enum leds_interface_spi_mode mode, union leds_interface_spi_func func)
{
  int err;

  interface->mode = mode;
  interface->func = func;
  interface->buf_size = leds_interface_spi_buffer_size(mode, 0); // XXX: count is unknown?!

  LOG_INFO("using buf_size=%u", interface->buf_size);

  // TODO: use larger buffer for more efficient transactions?
  if ((err = leds_interface_spi_master_init(interface, options))) {
    LOG_ERROR("leds_interface_spi_master_init");
    return err;
  }

  interface->gpio = options->gpio;

  return 0;
}

static uint32_t *leds_interface_spi_map_32bit(struct leds_interface_spi *interface, unsigned *offp)
{
  size_t len = (*offp) * sizeof(uint32_t);

  if (len >= interface->buf_size) {
    *offp = 0;

    if (leds_interface_spi_master_write(interface, len)) {
      LOG_ERROR("leds_interface_spi_master_write");
      return NULL;
    }
  }

  return &interface->buf.spi_mode_32bit[(*offp)++];
}

static int leds_interface_spi_write_32bit(struct leds_interface_spi *interface, uint32_t data, unsigned *offp)
{
  size_t len = (*offp) * sizeof(uint32_t);
  int err;

  if (len >= interface->buf_size) {
    *offp = 0;

    if ((err = leds_interface_spi_master_write(interface, len))) {
      LOG_ERROR("leds_interface_spi_master_write");
      return err;
    }
  }

  interface->buf.spi_mode_32bit[(*offp)++] = data;

  return 0;
}

static int leds_interface_spi_flush_32bit(struct leds_interface_spi *interface, unsigned *offp)
{
  size_t len = (*offp) * sizeof(uint32_t);
  int err;

  if (len > 0) {
    if ((err = leds_interface_spi_master_write(interface, len))) {
      LOG_ERROR("leds_interface_spi_master_write");
      return err;
    }
  }

  if ((err = leds_interface_spi_master_flush(interface))) {
    LOG_ERROR("leds_interface_spi_master_flush");
    return err;
  }

  return 0;
}

static int leds_interface_spi_tx_32bit(struct leds_interface_spi *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
{
  unsigned off = 0;
  int err;

  // start frame
  if ((err = leds_interface_spi_write_32bit(interface, LEDS_INTERFACE_SPI_MODE_32BIT_START_FRAME, &off))) {
    LOG_ERROR("leds_interface_spi_write_32bit");
    return err;
  }

  // pixel frames
  for (unsigned i = 0; i < count; i++) {
    uint32_t *ptr;

    if (!(ptr = leds_interface_spi_map_32bit(interface, &off))) {
      LOG_ERROR("leds_interface_spi_map_32bit");
      return -1;
    }

    interface->func.spi_mode_32bit(ptr, pixels, i, limit);
  }

  // end frames
  for (unsigned i = 0; i < leds_interface_spi_mode_32bit_end_frames(count); i++) {
    if ((err = leds_interface_spi_write_32bit(interface, LEDS_INTERFACE_SPI_MODE_32BIT_END_FRAME, &off))) {
      LOG_ERROR("leds_interface_spi_write_32bit");
      return err;
    }
  }

  if ((err = leds_interface_spi_flush_32bit(interface, &off))) {
    LOG_ERROR("leds_interface_spi_flush_32bit");
    return err;
  }

  return 0;
}

int leds_interface_spi_tx(struct leds_interface_spi *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
{
  struct leds_interface_spi_stats *stats = &leds_interface_stats.spi;
  int err;

  WITH_STATS_TIMER(&stats->open) {
    if ((err = leds_interface_spi_master_open(interface))) {
      LOG_ERROR("leds_interface_spi_master_open");
      return err;
    }
  }

#if CONFIG_LEDS_GPIO_ENABLED
  leds_gpio_setup(&interface->gpio);
#endif

  WITH_STATS_TIMER(&stats->tx) {
    switch(interface->mode) {
      case LEDS_INTERFACE_SPI_MODE0_32BIT:
      case LEDS_INTERFACE_SPI_MODE1_32BIT:
      case LEDS_INTERFACE_SPI_MODE2_32BIT:
      case LEDS_INTERFACE_SPI_MODE3_32BIT:
        if ((err = leds_interface_spi_tx_32bit(interface, pixels, count, limit))) {
          LOG_ERROR("leds_interface_spi_tx_32bit");
          goto error;
        }

        break;

      default:
        LOG_FATAL("invalid mode=%d", interface->mode);
    }
  }

error:
#if CONFIG_LEDS_GPIO_ENABLED
  leds_gpio_close(&interface->gpio);
#endif

  if ((err = leds_interface_spi_master_close(interface))) {
    LOG_ERROR("leds_interface_spi_master_close");
    return err;
  }

  return err;
}
