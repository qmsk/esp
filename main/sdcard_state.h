#pragma once

#include <sdkconfig.h>

#if CONFIG_SDCARD_ENABLED
  #include <driver/sdmmc_types.h>

  extern sdmmc_host_t *sdcard_host;
  extern sdmmc_card_t *sdcard_card;
#endif
