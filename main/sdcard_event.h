#pragma once

#include <sdkconfig.h>

#if CONFIG_SDCARD_ENABLED
  #include <freertos/FreeRTOS.h>
  #include <freertos/event_groups.h>
  
  enum sdcard_event {
    SDCARD_EVENT_CD_GPIO,
  };

  extern EventGroupHandle_t sdcard_events;
#endif
