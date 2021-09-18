#include "user_event.h"

#include "config.h"
#include "status_leds.h"
#include "system.h"

#include <logging.h>

static const char *user_event_str(enum user_event event)
{
  switch (event) {
    case USER_EVENT_BOOT:             return "BOOT";
    case USER_EVENT_CONNECTING:       return "CONNECTING";
    case USER_EVENT_CONNECTED:        return "CONNECTED";
    case USER_EVENT_DISCONNECTED:     return "DISCONNECTED";
    case USER_EVENT_RESET:            return "RESET";
    default:                          return "?";
  }
}

void user_event(enum user_event event)
{
  LOG_INFO("%s", user_event_str(event));

  status_led_event(event);
}

void user_reset()
{
  reset_config();
  system_restart();
}
