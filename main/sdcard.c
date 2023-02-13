#include "sdcard.h"
#include "sdcard_state.h"
#include "sdcard_spi.h"

#include <logging.h>

#if CONFIG_SDCARD_ENABLED
  #include <driver/sdmmc_types.h>

  sdmmc_card_t *sdcard_card;

  int init_sdcard()
  {
    int err;

    if (!(sdcard_card = malloc(sizeof(*sdcard_card)))) {
      LOG_ERROR("malloc");
      return -1;
    }

  #if CONFIG_SDCARD_SPI_HOST
    if ((err = init_sdcard_spi(sdcard_card))) {
      LOG_ERROR("init_sdcard_spi");
      return err;
    } else {
      LOG_INFO("SD SPI Card initialized");
    }
  #else
    #error "No SD Card host selected"
  #endif

    return 0;
  }
#endif
