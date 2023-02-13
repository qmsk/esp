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
    struct sdmmc_card_info card_info = sdmmc_card_get_info(card);

  #if CONFIG_SDCARD_SPI_HOST
    printf("Host: SPI\n");
  #endif
    printf("Type: %s\n", sdmmc_card_type_str(card_type));
    printf("ID: manufacturer %02x OEM [%.2s] product [%.5s] revision %d.%d serial %08d date %4d/%2d\n",
      card_info.mfg_id,
      card_info.oem_id,
      card_info.product,
      card_info.revision_major, card_info.revision_minor,
      card_info.serial,
      card_info.date_year, card_info.date_month
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
