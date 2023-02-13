#pragma once

#include <driver/sdmmc_types.h>

enum sdmmc_host_type {
  SDMMC_HOST_TYPE_NONE,
  SDMMC_HOST_TYPE_SDSPI,
  SDMMC_HOST_TYPE_SDMMC,
};

enum sdmmc_card_type {
  SDMMC_CARD_TYPE_NONE,
  SDMMC_CARD_TYPE_SDIO,
  SDMMC_CARD_TYPE_MMC,
  SDMMC_CARD_TYPE_SDHC,
  SDMMC_CARD_TYPE_SDSC,
};

struct sdmmc_card_info {
  uint8_t mfg_id;
  char oem_id[2];
  char product[5];
  unsigned revision_major : 4, revision_minor : 4;
  uint32_t serial;
  unsigned date_year : 12, date_month : 4;
};

enum sdmmc_host_type sdmmc_host_type(const sdmmc_host_t *host);

const char *sdmmc_host_type_str(enum sdmmc_host_type type);

enum sdmmc_card_type sdmmc_card_type(const sdmmc_card_t *card);

const char *sdmmc_card_type_str(enum sdmmc_card_type type);

static inline uint64_t sdmmc_card_get_size(const sdmmc_card_t *card)
{
  return ((uint64_t) card->csd.capacity) * ((uint64_t) card->csd.sector_size);
}

static inline uint32_t sdmmc_card_get_freq(const sdmmc_card_t *card)
{
  return ((uint32_t) card->max_freq_khz) * 1000;
}

struct sdmmc_card_info sdmmc_card_get_info(const sdmmc_card_t *card);

esp_err_t sdmmc_host_deinit(sdmmc_host_t *host);
