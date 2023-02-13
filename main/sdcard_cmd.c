#include "sdcard.h"
#include "sdcard_state.h"

#include <logging.h>

#include <stdio.h>

#if CONFIG_SDCARD_ENABLED
  #include <system_sdmmc.h>

  #include <sdmmc_cmd.h>

  int sdcard_status_cmd(int argc, char **argv, void *ctx)
  {
    esp_err_t err;

    if ((err = sdmmc_get_status(sdcard_card))) {
      LOG_ERROR("%s", esp_err_to_name(err));
      return -1;
    } else {
      printf("SD Card OK!\n");
      return 0;
    }
  }


  static void print_sdcard_info(const sdmmc_card_t* card)
  {
    enum sdmmc_card_type card_type = sdmmc_card_get_type(card);

  #if CONFIG_SDCARD_SPI_HOST
    printf("Host: SPI\n");
  #endif
    printf("Type: %s\n", sdmmc_card_type_str(card_type));
    printf("ID: manufacturer %02x OEM [%.2s] product [%.8s] revision %d.%d serial %08d date %4d/%2d\n",
      SDMMC_CARD_CID_MFG_ID(card),
      SDMMC_CARD_CID_OEM_ID(card),
      SDMMC_CARD_CID_NAME(card),
      SDMMC_CARD_CID_REVISON_MAJOR(card), SDMMC_CARD_CID_REVISON_MINOR(card),
      SDMMC_CARD_CID_SERIAL(card),
      SDMMC_CARD_CID_DATE_YEAR(card), SDMMC_CARD_CID_DATE_MONTH(card)
    );
    printf("Speed: %3.2fMHz%s\n",
      ((float) sdmmc_card_get_freq(card)) / ((float) (1000 * 1000)),
      card->is_ddr ? " (DDR)" : ""
    );
    printf("Size: %3.2fGB\n",
      ((float) sdmmc_card_get_size(card)) / ((float) (1024 * 1024 * 1024))
    );
  }

  int sdcard_info_cmd(int argc, char **argv, void *ctx)
  {
    print_sdcard_info(sdcard_card);

    return 0;
  }

  const struct cmd sdcard_commands[] = {
    { "status", sdcard_status_cmd,  .describe = "Show SD Card Status"  },
    { "info",   sdcard_info_cmd,    .describe = "Show SD Card Info"  },
    {}
  };

  const struct cmdtab sdcard_cmdtab = {
    .commands = sdcard_commands,
  };
#endif
