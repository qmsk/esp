#include "../sdcard_spi.h"

#include <logging.h>

#if CONFIG_SDCARD_SPI_HOST
  #include <driver/spi_master.h>
  #include <driver/sdspi_host.h>
  #include <driver/sdmmc_types.h>
  #include <sdmmc_cmd.h>

  // TODO: shared? conflict with leds_spi?
  #define SDCARD_SPI_HOST SPI2_HOST
  #define SDCARD_SPI_DMA_CH SPI_DMA_CH_AUTO
  #define SDCARD_SPI_MOSI_IO_NUM 15
  #define SDCARD_SPI_MISO_IO_NUM 2
  #define SDCARD_SPI_SCLK_IO_NUM 14
  #define SDCARD_SPI_CS_IO_NUM 13
  #define SDCARD_SPI_MAX_TRANSFER_SZ 4000 // XXX: whence? One DMA buffer?
  #define SDCARD_SPI_MAX_FREQ_KHZ 10000 // 10MHz for crappy breadboard testing

  sdspi_dev_handle_t sdcard_sdspi_dev;

  int init_sdcard_spi(sdmmc_card_t *card)
  {
    spi_bus_config_t bus_config = {
      .mosi_io_num      = SDCARD_SPI_MOSI_IO_NUM,
      .miso_io_num      = SDCARD_SPI_MISO_IO_NUM,
      .sclk_io_num      = SDCARD_SPI_SCLK_IO_NUM,
      .quadwp_io_num    = -1,
      .quadhd_io_num    = -1,
      .max_transfer_sz  = SDCARD_SPI_MAX_TRANSFER_SZ,
    };
    sdspi_device_config_t dev_config = {
      .host_id          = SDCARD_SPI_HOST,
      .gpio_cs          = SDCARD_SPI_CS_IO_NUM,
      .gpio_cd          = SDSPI_SLOT_NO_CD,
      .gpio_wp          = SDSPI_SLOT_NO_WP,
      .gpio_int         = SDSPI_SLOT_NO_INT,
    };
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    esp_err_t err;

    if ((err = spi_bus_initialize(SDCARD_SPI_HOST, &bus_config, SDCARD_SPI_DMA_CH))) {
      LOG_ERROR("spi_bus_initialize(%d): %s", SDCARD_SPI_HOST, esp_err_to_name(err));
      return -1;
    }

    if ((err = sdspi_host_init())) {
      LOG_ERROR("sdspi_host_init: %s", esp_err_to_name(err));
      return -1;
    }

    if ((err = sdspi_host_init_device(&dev_config, &sdcard_sdspi_dev))) {
      LOG_ERROR("sdspi_host_init_device: %s", esp_err_to_name(err));
      return -1;
    } else {
      host.slot = sdcard_sdspi_dev;
      host.max_freq_khz = SDCARD_SPI_MAX_FREQ_KHZ;
    }

    if ((err = sdmmc_card_init(&host, card))) {
      if (err == ESP_ERR_TIMEOUT) {
        LOG_WARN("sdmmc_card_init: timeout, card present?");
        return 1;
      } else {
        LOG_ERROR("sdmmc_card_init: %s", esp_err_to_name(err));
        return -1;
      }
    }

    return 0;
  }
#endif
