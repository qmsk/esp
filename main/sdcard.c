#include "sdcard.h"
#include "sdcard_event.h"
#include "sdcard_fatfs.h"
#include "sdcard_state.h"
#include "sdcard_spi.h"
#include "tasks.h"

#include <logging.h>

#if CONFIG_SDCARD_ENABLED
  #include <driver/sdmmc_types.h>
  #include <sdmmc_cmd.h>

  #include <freertos/FreeRTOS.h>
  #include <freertos/event_groups.h>
  #include <freertos/task.h>

  #define SDCARD_TASK_POLL_INTERVAL (1000 / portTICK_PERIOD_MS)

  xTaskHandle sdcard_task;
  EventGroupHandle_t sdcard_events;
  sdmmc_host_t *sdcard_host;
  sdmmc_card_t *sdcard_card;

  static int get_sdcard_cd()
  {
    #if CONFIG_SDCARD_SPI_HOST
      return get_sdcard_spi_cd();
    #else
      #error "No SD Card host selected"
    #endif
  }

  void sdcard_main(void *arg)
  {
    for (;;) {
      TickType_t wait_ticks = SDCARD_TASK_POLL_INTERVAL;
      const bool clear_on_exit = true;
      const bool wait_for_all_bits = false;

      EventBits_t event_bits = xEventGroupWaitBits(sdcard_events, (1 << SDCARD_EVENT_CD_GPIO), clear_on_exit, wait_for_all_bits, wait_ticks);
      int cd = get_sdcard_cd();

      LOG_DEBUG("event_bits=%08x cd=%d", event_bits, cd);

      if (cd < 0) {
        LOG_ERROR("failed sdcard cd");
      } else if (cd && !sdcard_card) {
        LOG_INFO("hotplug sdcard...");

        // TODO: delay, initial attempt is likely to fail due to nature of mechanical CD switch?
        if (start_sdcard()) {
          LOG_ERROR("start_sdcard");
        }
      } else if (!cd && sdcard_card) {
        LOG_INFO("hot-unplug sdcard...");

        if (stop_sdcard()) {
          LOG_ERROR("stop_sdcard");
        }
      }
    }
  }

  int init_sdcard()
  {
    struct task_options task_options = {
      .main       = sdcard_main,
      .name       = SDCARD_TASK_NAME,
      .stack_size = SDCARD_TASK_STACK,
      .arg        = NULL,
      .priority   = SDCARD_TASK_PRIORITY,
      .handle     = &sdcard_task,
      .affinity   = SDCARD_TASK_AFFINITY,
    };
    int err;

    if (!(sdcard_events = xEventGroupCreate())) {
      LOG_ERROR("xEventGroupCreate");
      return -1;
    }

  #if CONFIG_SDCARD_SPI_HOST
    if ((err = init_sdcard_spi(&sdcard_host))) {
      LOG_ERROR("init_sdcard_spi");
      return err;
    }
  #else
    #error "No SD Card host selected"
  #endif

    // attempt to start immediately, otherwise hotplug later from task
    if ((err = start_sdcard())) {
      LOG_WARN("start_sdcard");
    }

    // task
    if (start_task(task_options)) {
      LOG_ERROR("start_task");
      return -1;
    } else {
      LOG_INFO("start task=%p", sdcard_task);
    }

    return 0;
  }

  int start_sdcard()
  {
    sdmmc_card_t *card;
    esp_err_t err;
    int ret;

    if (!sdcard_host) {
      LOG_ERROR("host not initialized");
      return -1;
    }

    if (sdcard_card) {
      LOG_ERROR("card already initialzed");
      return 0;
    }

    if (!(card = malloc(sizeof(*card)))) {
      LOG_ERROR("malloc");
      return -1;
    }

    if ((err = sdmmc_card_init(sdcard_host, card))) {
      if (err == ESP_ERR_NOT_FOUND) {
        LOG_WARN("sdmmc_card_init: %s, card not detected?", esp_err_to_name(err));
        ret = 1;
        goto error;
      } else if (err == ESP_ERR_TIMEOUT) {
        LOG_WARN("sdmmc_card_init: %s, card not present?", esp_err_to_name(err));
        ret = 1;
        goto error;
      } else {
        LOG_ERROR("sdmmc_card_init: %s", esp_err_to_name(err));
        ret = -1;
        goto error;
      }
    } else {
      sdcard_card = card;
    }

    if ((ret = mount_sdcard_fatfs(card))) {
      LOG_ERROR("mount_sdcard_fatfs");
      return ret;
    }

    return 0;
error:
    free(card);

    return ret;
  }

  int stop_sdcard()
  {
    sdmmc_card_t *card = sdcard_card;
    int err;

    if ((err = unmount_sdcard_fatfs(card))) {
      LOG_ERROR("unmount_sdcard_fatfs");
      return err;
    }

    sdcard_card = NULL;

    if (card) {
      // do not de-initialize the host, keep it ready for start_sdcard()
      free(card);
    }

    return 0;
  }

#endif
