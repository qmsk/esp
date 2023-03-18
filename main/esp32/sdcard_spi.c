#include "../sdcard_spi.h"
#include "../sdcard_event.h"

#include <logging.h>

#if CONFIG_SDCARD_SPI_HOST
  #include <driver/spi_master.h>
  #include <driver/sdspi_host.h>
  #include <driver/sdmmc_types.h>

  #include <gpio.h>

  #define SDCARD_SPI_HOST SPI2_HOST
  #define SDCARD_SPI_DMA_CH SPI_DMA_CH_AUTO
  #define SDCARD_SPI_MAX_TRANSFER_SZ 4000 // XXX: whence? One DMA buffer?
  #define SDCARD_SPI_MAX_FREQ_KHZ CONFIG_SDCARD_SPI_MAX_FREQ_KHZ

  #if CONFIG_SDCARD_SPI_CD_HOST
    #define SDCARD_SPI_GPIO_TYPE GPIO_TYPE_HOST

    #define SDCARD_SPI_GPIO_CD_PINS GPIO_PINS(CONFIG_SDCARD_SPI_CD_PIN)
    #define SDCARD_SPI_CD_PIN CONFIG_SDCARD_SPI_CD_PIN

    #if CONFIG_SDCARD_SPI_CD_POL_NO
      #define SDCARD_SPI_GPIO_CD_INV_PINS GPIO_PINS(CONFIG_SDCARD_SPI_CD_PIN)
      #define SDCARD_SPI_CD_INV 0
    #elif CONFIG_SDCARD_SPI_CD_POL_NC
      #define SDCARD_SPI_GPIO_CD_INV_PINS 0
      #define SDCARD_SPI_CD_INV 1
    #endif
  #else
    #define SDCARD_SPI_CD_PIN SDSPI_SLOT_NO_CD
    #define SDCARD_SPI_CD_INV 0
  #endif
  #define SDCARD_SPI_WP_PIN SDSPI_SLOT_NO_WP
  #define SDCARD_SPI_INT_PIN SDSPI_SLOT_NO_INT

  // XXX: GPIO39 GPIO isr seems to be broken?
  static IRAM_ATTR void sdcard_spi_gpio_intr_handler(gpio_pins_t pins, void *arg)
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    LOG_ISR_DEBUG("pins=%08x", pins);

    xEventGroupSetBitsFromISR(sdcard_events, (1 << SDCARD_EVENT_CD_GPIO), &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }

  struct gpio_options sdcard_spi_gpio_options = {
  #if defined(SDCARD_SPI_GPIO_TYPE)
    .type           = SDCARD_SPI_GPIO_TYPE,
    .in_pins        = (SDCARD_SPI_GPIO_CD_PINS),
    .inverted_pins  = (SDCARD_SPI_GPIO_CD_INV_PINS),
    .interrupt_pins = (SDCARD_SPI_GPIO_CD_PINS),
    .interrupt_func = sdcard_spi_gpio_intr_handler,
  #endif
  };
  sdmmc_host_t sdcard_spi_host = SDSPI_HOST_DEFAULT();

  int init_sdcard_spi_gpio()
  {
    const struct gpio_options *options = &sdcard_spi_gpio_options;
    gpio_pins_t pins;
    int err;

    LOG_INFO("in_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT " interrupt_pins=" GPIO_PINS_FMT,
      GPIO_PINS_ARGS(options->in_pins),
      GPIO_PINS_ARGS(options->inverted_pins),
      GPIO_PINS_ARGS(options->interrupt_pins)
    );

    if ((err = gpio_setup(options))) {
      LOG_ERROR("gpio_setup");
      return err;
    }

    gpio_in_get(options, &pins);

    LOG_INFO("pins=" GPIO_PINS_FMT, GPIO_PINS_ARGS(pins));

    return 0;
  }

  int init_sdcard_spi(sdmmc_host_t **hostp)
  {
    spi_bus_config_t bus_config = {
      .mosi_io_num      = CONFIG_SDCARD_SPI_MOSI_PIN,
      .miso_io_num      = CONFIG_SDCARD_SPI_MISO_PIN,
      .sclk_io_num      = CONFIG_SDCARD_SPI_SCLK_PIN,
      .quadwp_io_num    = -1,
      .quadhd_io_num    = -1,
      .max_transfer_sz  = SDCARD_SPI_MAX_TRANSFER_SZ,
    };
    sdspi_device_config_t dev_config = {
      .host_id          = SDCARD_SPI_HOST,
      .gpio_cs          = CONFIG_SDCARD_SPI_CS_PIN,
      .gpio_cd          = SDCARD_SPI_CD_PIN,
      .gpio_cd_inv      = SDCARD_SPI_CD_INV,
      .gpio_wp          = SDCARD_SPI_WP_PIN,
      .gpio_int         = SDCARD_SPI_INT_PIN,
    };
    sdspi_dev_handle_t dev;
    esp_err_t err;

    if ((err = spi_bus_initialize(SDCARD_SPI_HOST, &bus_config, SDCARD_SPI_DMA_CH))) {
      LOG_ERROR("spi_bus_initialize(%d): %s", SDCARD_SPI_HOST, esp_err_to_name(err));
      return -1;
    }

    if ((err = sdspi_host_init())) {
      LOG_ERROR("sdspi_host_init: %s", esp_err_to_name(err));
      return -1;
    }

    if ((err = sdspi_host_init_device(&dev_config, &dev))) {
      LOG_ERROR("sdspi_host_init_device: %s", esp_err_to_name(err));
      return -1;
    } else {
      sdcard_spi_host.slot = dev;
      sdcard_spi_host.max_freq_khz = SDCARD_SPI_MAX_FREQ_KHZ;
    }

    LOG_INFO("using spi_host=%d slot=%d max_freq_khz=%d",
      SDCARD_SPI_HOST,
      sdcard_spi_host.slot,
      sdcard_spi_host.max_freq_khz
    );

    if ((err = init_sdcard_spi_gpio())) {
      return err;
    }

    *hostp = &sdcard_spi_host;

    return 0;
  }

  int get_sdcard_spi_cd()
  {
  #if defined(SDCARD_SPI_GPIO_TYPE)
    gpio_pins_t pins;

    if (gpio_in_get(&sdcard_spi_gpio_options, &pins)) {
      LOG_ERROR("gpio_in_get");
      return -1;
    }

    if (pins & SDCARD_SPI_GPIO_CD_PINS) {
      return 1;
    }

    return 0;
  #else
    // unknown, assume present
    return 1;
  #endif
  }
#endif
