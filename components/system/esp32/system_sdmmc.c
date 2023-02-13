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
