#include <system_sdmmc.h>

#include <driver/sdmmc_defs.h>

enum sdmmc_card_type sdmmc_card_get_type(const sdmmc_card_t* card)
{
  if (!card) {
    return SDMMC_CARD_TYPE_NONE;
  } else if (card->is_sdio) {
    return SDMMC_CARD_TYPE_SDIO;
  } else if (card->is_mmc) {
    return SDMMC_CARD_TYPE_MMC;
  } else if (card->ocr & SD_OCR_SDHC_CAP) {
    return SDMMC_CARD_TYPE_SDHC;
  } else {
    return SDMMC_CARD_TYPE_SDSC;
  }
}

const char *sdmmc_card_type_str(enum sdmmc_card_type type)
{
  switch(type) {
    case SDMMC_CARD_TYPE_NONE:  return "NONE";
    case SDMMC_CARD_TYPE_SDIO:  return "SDIO";
    case SDMMC_CARD_TYPE_MMC:   return "MMC";
    case SDMMC_CARD_TYPE_SDHC:  return "SDHC";
    case SDMMC_CARD_TYPE_SDSC:  return "SDSC";
    default:                    return "?";
  };
}

struct sdmmc_card_info sdmmc_card_get_info(const sdmmc_card_t *card)
{
  return (struct sdmmc_card_info) {
    .mfg_id = card->cid.mfg_id,
    .oem_id = {
      (card->cid.oem_id >> 8) & 0xff,
      (card->cid.oem_id >> 0) & 0xff
    },

    .product = {
      card->cid.name[0],
      card->cid.name[1],
      card->cid.name[2],
      card->cid.name[3],
      card->cid.name[4],
    },

    .revision_major = (card->cid.revision >> 4) & 0xf,
    .revision_minor = (card->cid.revision >> 0) & 0xf,

    .serial = card->cid.serial,

    .date_year  = 2000 + ((card->cid.date >> 4) & 0xff),
    .date_month = (card->cid.date >> 0) & 0xf,
  };
}
