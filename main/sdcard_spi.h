#pragma once

#include <sdkconfig.h>

#if CONFIG_SDCARD_SPI_HOST
  #include <driver/sdmmc_types.h>

  int init_sdcard_spi(sdmmc_host_t **hostp);
#endif
