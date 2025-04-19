#include "user.h"
#include "user_leds.h"
#include "user_log.h"

#include <logging.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct user_log user_power_log;
struct user_log user_state_log;
struct user_log user_activity_log;
struct user_log user_alert_log;

const char *user_power_str(enum user_power power)
{
  switch (power) {
    case USER_POWER_INIT:             return "INIT";
    case USER_POWER_ON:               return "ON";
    default:                          return NULL;
  }
}

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
    case USER_ACTIVITY_IDLE:    return "IDLE";

    case USER_ACTIVITY_LEDS:                    return "LEDS";
    case USER_ACTIVITY_LEDS_CMD:                return "LEDS_CMD";
    case USER_ACTIVITY_LEDS_HTTP:               return "LEDS_HTTP";
    case USER_ACTIVITY_LEDS_SEQUENCE:           return "LEDS_SEQUENCE";
    case USER_ACTIVITY_LEDS_ARTNET:             return "LEDS_ARTNET";
    case USER_ACTIVITY_LEDS_ARTNET_TIMEOUT:     return "LEDS_ARTNET_TIMEOUT";
    case USER_ACTIVITY_LEDS_TEST:               return "LEDS_TEST";

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

void init_user_power()
{
  user_power_log = (struct user_log) {
    .tick   = xTaskGetTickCount(),
    .state  = USER_POWER_ON,
  };
}

int init_user()
{
  init_user_power();

  return 0;
}

void user_power(enum user_power power)
{
  const char *str = user_power_str(power);

  if (str) {
    LOG_INFO("%s", str);
  } else {
    LOG_INFO("%d", power);
  }

  user_power_log = (struct user_log) {
    .tick   = xTaskGetTickCount(),
    .power  = power,
  };

  // TODO: set_user_leds_power(state);
}

void user_state(enum user_state state)
{
  const char *str = user_state_str(state);

  if (str) {
    LOG_INFO("%s", str);
  } else {
    LOG_INFO("%d", state);
  }

  user_state_log = (struct user_log) {
    .tick   = xTaskGetTickCount(),
    .state  = state,
  };

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

  user_activity_log = (struct user_log) {
    .tick     = xTaskGetTickCount(),
    .activity = activity,
  };

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

  user_alert_log = (struct user_log) {
    .tick     = xTaskGetTickCount(),
    .alert    = alert,
  };

  set_user_leds_alert(alert);
}
