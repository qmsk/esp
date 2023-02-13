#pragma once

#include <driver/sdmmc_types.h>

enum sdmmc_card_type {
  SDMMC_CARD_TYPE_NONE,
  SDMMC_CARD_TYPE_SDIO,
  SDMMC_CARD_TYPE_MMC,
  SDMMC_CARD_TYPE_SDHC,
  SDMMC_CARD_TYPE_SDSC,
};

enum sdmmc_card_type sdmmc_card_get_type(const sdmmc_card_t *card);

const char *sdmmc_card_type_str(enum sdmmc_card_type type);

static inline uint64_t sdmmc_card_get_size(const sdmmc_card_t *card)
{
  return ((uint64_t) card->csd.capacity) * ((uint64_t) card->csd.sector_size);
}

static inline uint32_t sdmmc_card_get_freq(const sdmmc_card_t *card)
{
  return ((uint32_t) card->max_freq_khz) * 1000;
}

#define SDMMC_CARD_CID_MFG_ID(card) ((card)->cid.mfg_id)
#define SDMMC_CARD_CID_OEM_ID(card) ((char[2]){ (((card)->cid.oem_id >> 8) & 0xff), (((card)->cid.oem_id >> 0) & 0xff) })
#define SDMMC_CARD_CID_NAME(card) ((card)->cid.name)
#define SDMMC_CARD_CID_REVISON_MAJOR(card) (((card)->cid.revision >> 4) & 0xf)
#define SDMMC_CARD_CID_REVISON_MINOR(card) (((card)->cid.revision >> 0) & 0xf)
#define SDMMC_CARD_CID_SERIAL(card) ((card)->cid.serial)
#define SDMMC_CARD_CID_DATE_YEAR(card) (2000 + (((card)->cid.date >> 4) & 0xff))
#define SDMMC_CARD_CID_DATE_MONTH(card) (((card)->cid.date >> 0) & 0xf)
