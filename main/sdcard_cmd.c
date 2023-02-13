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

    printf("Host: %s\n", sdmmc_host_type_str(sdmmc_host_type(sdcard_host)));
    printf("Card: %s\n", sdmmc_card_type_str(sdmmc_card_type(sdcard_card)));

    if (sdcard_card) {
      if ((err = sdmmc_get_status(sdcard_card))) {
        printf("Card Status: %s", esp_err_to_name(err));
        return -1;
      } else {
        printf("Card Status: OK\n");
      }
    }

    return 0;
  }

  static void print_sdcard_info(const sdmmc_card_t* card)
  {
    enum sdmmc_host_type host_type = sdmmc_host_type(&card->host);
    enum sdmmc_card_type card_type = sdmmc_card_type(card);

    struct sdmmc_card_info card_info = sdmmc_card_get_info(card);

    printf("Host: %s\n", sdmmc_host_type_str(host_type));
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
    if (sdcard_card) {
      print_sdcard_info(sdcard_card);
    }

    return 0;
  }

  int sdcard_start_cmd(int argc, char **argv, void *ctx)
  {
    int err;

    if ((err = start_sdcard())) {
      LOG_ERROR("start_sdcard");
      return -1;
    }

    return 0;
  }
  int sdcard_stop_cmd(int argc, char **argv, void *ctx)
  {
    int err;

    if ((err = stop_sdcard())) {
      LOG_ERROR("stop_sdcard");
      return -1;
    }

    return 0;
  }

  const struct cmd sdcard_commands[] = {
    { "status", sdcard_status_cmd,  .describe = "Show SD Card Status"  },
    { "start",  sdcard_start_cmd,   .describe = "Start SD Card"        },
    { "info",   sdcard_info_cmd,    .describe = "Show SD Card Info"    },
    { "stop",   sdcard_stop_cmd ,   .describe = "Stop SD Card"        },
    {}
  };

  const struct cmdtab sdcard_cmdtab = {
    .commands = sdcard_commands,
  };
#endif
