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

  #define SDCARD_POLL_TICKS (1000 / portTICK_PERIOD_MS)
  #define SDCARD_HOTPLUG_TICKS (3000 / portTICK_PERIOD_MS)

  xTaskHandle sdcard_task;
  EventGroupHandle_t sdcard_events;
  TickType_t sdcard_hotplug_tick = 0;
  sdmmc_host_t *sdcard_host;
  sdmmc_card_t *sdcard_card;

  void check_sdcard_hotplug()
  {
    int err;
    TickType_t hotplug_ticks;

    if (sdcard_card) {
      // already mounted
      return;
    }

    if (!sdcard_hotplug_tick) {
      sdcard_hotplug_tick = xTaskGetTickCount();
      hotplug_ticks = 0;
    } else {
      hotplug_ticks = xTaskGetTickCount() - sdcard_hotplug_tick;
    }

    if (!hotplug_ticks) {
      LOG_INFO("detected sdcard...");
    } else if (hotplug_ticks >= SDCARD_HOTPLUG_TICKS) {
      LOG_INFO("hotplug sdcard...");

      if ((err = start_sdcard()) < 0) {
        LOG_ERROR("start_sdcard");
      } else if (err) {
        LOG_WARN("start_sdcard");
      } else {
        LOG_INFO("start_sdcard");

        sdcard_hotplug_tick = 0;
      }
    }
  }

  void ensure_sdcard_stopped()
  {
    if (sdcard_hotplug_tick) {
      LOG_INFO("cancel hotplug...");
      sdcard_hotplug_tick = 0;
    }

    if (sdcard_card) {
      LOG_INFO("hot-unplug sdcard...");

      if (stop_sdcard()) {
        LOG_ERROR("stop_sdcard");
      }
    }
  }

  void sdcard_main(void *arg)
  {
    for (;;) {
      TickType_t wait_ticks = SDCARD_POLL_TICKS;
      const bool clear_on_exit = true;
      const bool wait_for_all_bits = false;
      esp_err_t err;

      EventBits_t event_bits = xEventGroupWaitBits(sdcard_events, (1 << SDCARD_EVENT_CD), clear_on_exit, wait_for_all_bits, wait_ticks);
      bool card_detect = false;

      if ((err = sdmmc_card_detect(sdcard_host))) {
        if (err == ESP_ERR_NOT_SUPPORTED) {
          LOG_DEBUG("no CD, assume present");
          card_detect = true;
        } else if (err == ESP_ERR_NOT_FOUND) {
          LOG_DEBUG("CD not asserted");
          card_detect = false;
        }
      } else {
        LOG_DEBUG("CD asserted");

        card_detect = true;
      }

      LOG_DEBUG("event_bits=%08x card_detect=%d", event_bits, card_detect);

      if (card_detect) {
        check_sdcard_hotplug();
      } else {
        ensure_sdcard_stopped();
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
