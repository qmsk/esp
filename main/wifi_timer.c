#include "wifi.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#define WIFI_TIMER_NAME "wifi-timer"
#define WIFI_TIMER_PERIOD ( 1000 / portTICK_PERIOD_MS )


bool _wifi_reconnect;
TimerHandle_t _wifi_timer;

void enable_wifi_reconnect()
{
  _wifi_reconnect = true;
}

void disable_wifi_reconnect()
{
    _wifi_reconnect = false;
}

void wifi_timer(TimerHandle_t timer)
{
  if (!_wifi_reconnect) {
    LOG_WARN("reconnect disabled");
    return;
  }

  LOG_INFO("reconnecting");

  if (wifi_reconnect()) {
    LOG_ERROR("wifi_reconnect");
  }
}

void start_wifi_reconnect()
{
  if (!_wifi_reconnect) {
      LOG_INFO("reconnect disabled");
      return;
  }

  LOG_INFO("reconnect in %d ticks", WIFI_TIMER_PERIOD);

  if (!xTimerStart(_wifi_timer, 0)) {
    LOG_ERROR("xTimerStart");
    return;
  }
}

void cancel_wifi_reconnect()
{
  if (!xTimerStop(_wifi_timer, 0)) {
    LOG_ERROR("xTimerStop");
    return;
  }
}

int init_wifi_timer()
{
  if (!(_wifi_timer = xTimerCreate(WIFI_TIMER_NAME, WIFI_TIMER_PERIOD, false, NULL, &wifi_timer))) {
    LOG_ERROR("xTimerCreate");
    return -1;
  }

  return 0;
}
