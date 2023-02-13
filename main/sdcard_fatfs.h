#pragma once

#include <sdkconfig.h>

#if CONFIG_SDCARD_ENABLED
  #include <driver/sdmmc_types.h>

  int mount_sdcard_fatfs(sdmmc_card_t *card);
#endif
