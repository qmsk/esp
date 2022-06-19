#pragma once

enum user_state {
  // states
  USER_STATE_BOOT,
  USER_STATE_CONNECTING,
  USER_STATE_CONNECTED,
  USER_STATE_DISCONNECTING,
  USER_STATE_DISCONNECTED,
  USER_STATE_STOPPED,
  USER_STATE_RESET,

  USER_STATE_MAX
};

enum user_activity {
  USER_ACTIVITY_LEDS,
  USER_ACTIVITY_DMX_INPUT,
  USER_ACTIVITY_DMX_OUTPUT,

  USER_ACTIVITY_MAX
};

enum user_alert {
  USER_ALERT_ERROR_BOOT,
  USER_ALERT_ERROR_CONFIG,
  USER_ALERT_ERROR_SETUP,
  USER_ALERT_ERROR_WIFI,
  USER_ALERT_ERROR_START,
  USER_ALERT_ERROR_DMX,
  USER_ALERT_MAX
};

// distribute events
void user_state(enum user_state state);
void user_activity(enum user_activity activity);
void user_alert(enum user_alert alert);

// user actions
void user_config_disable();
void user_config_mode();
void user_config_reset();

void user_test_trigger();
void user_test_hold();
void user_test_cancel();
