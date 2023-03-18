#include "sdcard.h"
#include "sdcard_fatfs.h"
#include "sdcard_state.h"
#include "sdcard_spi.h"

#include <logging.h>

#if CONFIG_SDCARD_ENABLED
  #include <driver/sdmmc_types.h>
  #include <sdmmc_cmd.h>

  sdmmc_host_t *sdcard_host;
  sdmmc_card_t *sdcard_card;

  int init_sdcard()
  {
    int err;

  #if CONFIG_SDCARD_SPI_HOST
    if ((err = init_sdcard_spi(&sdcard_host))) {
      LOG_ERROR("init_sdcard_spi");
      return err;
    }
  #else
    #error "No SD Card host selected"
  #endif

    // attempt to start, TODO: hotplug
    if ((err = start_sdcard())) {
      LOG_WARN("start_sdcard");
    }

    return 0;
  }

  int start_sdcard()
  {
    sdmmc_card_t *card;
    esp_err_t err;
    int ret;

    if (!sdcard_host) {
      LOG_ERROR("host not initialized");
      return -1;
    }

    if (sdcard_card) {
      LOG_ERROR("card already initialzed");
      return 0;
    }

    if (!(card = malloc(sizeof(*card)))) {
      LOG_ERROR("malloc");
      return -1;
    }

    if ((err = sdmmc_card_init(sdcard_host, card))) {
      if (err == ESP_ERR_NOT_FOUND) {
        LOG_WARN("sdmmc_card_init: %s, card not detected?", esp_err_to_name(err));
        ret = 1;
        goto error;
      } else if (err == ESP_ERR_TIMEOUT) {
        LOG_WARN("sdmmc_card_init: %s, card not present?", esp_err_to_name(err));
        ret = 1;
        goto error;
      } else {
        LOG_ERROR("sdmmc_card_init: %s", esp_err_to_name(err));
        ret = -1;
        goto error;
      }
    } else {
      sdcard_card = card;
    }

    if ((ret = mount_sdcard_fatfs(card))) {
      LOG_ERROR("mount_sdcard_fatfs");
      return ret;
    }

    return 0;
error:
    free(card);

    return ret;
  }

  int stop_sdcard()
  {
    sdmmc_card_t *card = sdcard_card;
    int err;

    if ((err = unmount_sdcard_fatfs(card))) {
      LOG_ERROR("unmount_sdcard_fatfs");
      return err;
    }

    sdcard_card = NULL;

    if (card) {
      // do not de-initialize the host, keep it ready for start_sdcard()
      free(card);
    }

    return 0;
  }

#endif
