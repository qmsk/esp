#include "user_event.h"

#include "artnet.h"
#include "config.h"
#include "status_leds.h"
#include "system.h"

#include <logging.h>

static const char *user_state_str(enum user_state state)
{
  switch (state) {
    case USER_STATE_BOOT:             return "BOOT";
    case USER_STATE_CONNECTING:       return "CONNECTING";
    case USER_STATE_CONNECTED:        return "CONNECTED";
    case USER_STATE_DISCONNECTED:     return "DISCONNECTED";
    case USER_STATE_RESET:            return "RESET";
    default:                          return NULL;
  }
}

static const char *user_activity_str(enum user_activity activity)
{
  switch (activity) {
    default:                          return NULL;
  }
}

static const char *user_alert_str(enum user_alert alert)
{
  switch (alert) {
    case USER_ALERT_ERROR_BOOT:       return "ERROR_BOOT";
    case USER_ALERT_ERROR_CONFIG:     return "ERROR_CONFIG";
    case USER_ALERT_ERROR_SETUP:      return "ERROR_SETUP";
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

  status_leds_state(state);
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

  status_leds_activity(activity);
}

void user_alert(enum user_alert alert)
{
  const char *str = user_alert_str(alert);

  if (str) {
    LOG_WARN("%s", str);
  } else {
    LOG_WARN("%d", alert);
  }

  status_leds_alert(alert);
}

void update_user_ipv4_address(ip4_addr_t ip_addr){
  LOG_INFO("ip_addr=%d.%d.%d.%d",
    ip4_addr1_val(ip_addr),
    ip4_addr2_val(ip_addr),
    ip4_addr3_val(ip_addr),
    ip4_addr4_val(ip_addr)
  );

  if (update_artnet()) {
    LOG_WARN("update_artnet");
  }
}

void user_reset()
{
  reset_config();
  system_restart();
}
