#include "user.h"
#include "user_leds.h"

#include "config.h"
#include "console.h"
#include "leds_test.h"
#include "system.h"
#include "wifi.h"

#include <logging.h>

const char *user_state_str(enum user_state state)
{
  switch (state) {
    case USER_STATE_INIT:             return "INIT";
    case USER_STATE_BOOT:             return "BOOT";
    case USER_STATE_CONNECTING:       return "CONNECTING";
    case USER_STATE_CONNECTED:        return "CONNECTED";
    case USER_STATE_DISCONNECTING:    return "DISCONNECTING";
    case USER_STATE_DISCONNECTED:     return "DISCONNECTED";
    case USER_STATE_STOPPED:          return "STOPPED";
    case USER_STATE_RESET:            return "RESET";
    default:                          return NULL;
  }
}

const char *user_activity_str(enum user_activity activity)
{
  switch (activity) {
    case USER_ACTIVITY_IDLE:          return "IDLE";
    case USER_ACTIVITY_LEDS:          return "LEDS";
    case USER_ACTIVITY_DMX_INPUT:     return "DMX_INPUT";
    case USER_ACTIVITY_DMX_OUTPUT:    return "DMX_OUTPUT";

    default:                          return NULL;
  }
}

const char *user_alert_str(enum user_alert alert)
{
  switch (alert) {
    case USER_ALERT_NONE:             return "NONE";

    case USER_ALERT_ERROR_BOOT:       return "ERROR_BOOT";
    case USER_ALERT_ERROR_CONFIG:     return "ERROR_CONFIG";
    case USER_ALERT_ERROR_SETUP:      return "ERROR_SETUP";
    case USER_ALERT_ERROR_WIFI:       return "ERROR_WIFI";
    case USER_ALERT_ERROR_START:      return "ERROR_START";
    case USER_ALERT_ERROR_DMX:        return "ERROR_DMX";
    
    case USER_ALERT_ERROR_LEDS_SEQUENCE:        return "ERROR_LEDS_SEQUENCE";
    case USER_ALERT_ERROR_LEDS_SEQUENCE_READ:   return "ERROR_LEDS_SEQUENCE_READ";
    case USER_ALERT_ERROR_ATX_PSU_TIMEOUT:      return "ERROR_ATX_PSU_TIMEOUT";

    default:                          return NULL;
  }
}

void user_state(enum user_state state)
{
  const char *str = user_state_str(state);

  if (str) {
    LOG_INFO("%s", str);
  } else {
    LOG_INFO("%d", state);
  }

  set_user_leds_state(state);
}

void user_activity(enum user_activity activity)
{
  const char *str = user_activity_str(activity);

  // activity is expected to be verbose
  if (str) {
    LOG_DEBUG("%s", str);
  } else {
    LOG_DEBUG("%05x", activity);
  }

  set_user_leds_activity(activity);
}

void user_alert(enum user_alert alert)
{
  const char *str = user_alert_str(alert);

  if (str) {
    LOG_WARN("%s", str);
  } else {
    LOG_WARN("%d", alert);
  }

  set_user_leds_alert(alert);
}

void user_config_disable()
{
  disable_config();
}

void user_config_mode()
{
  if (start_console() < 0) {
    user_alert(USER_ALERT_ERROR_START);
  }

  if (start_wifi_config()) {
    user_alert(USER_ALERT_ERROR_WIFI);
  }
}

void user_config_reset()
{
  reset_config();
  system_restart();
}

void user_test_trigger()
{
  trigger_leds_test();
}

void user_test_hold()
{
  auto_leds_test();
}

void user_test_cancel()
{
  reset_leds_test();
}
