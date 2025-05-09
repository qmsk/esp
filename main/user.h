#pragma once

enum user_power {
  USER_POWER_INIT = 0,

  USER_POWER_ON,

  USER_POWER_MAX
};

enum user_state {
  USER_STATE_INIT = 0,

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
  USER_ACTIVITY_IDLE = 0,

  USER_ACTIVITY_LEDS,
  USER_ACTIVITY_LEDS_CMD,
  USER_ACTIVITY_LEDS_HTTP,
  USER_ACTIVITY_LEDS_SEQUENCE,
  USER_ACTIVITY_LEDS_ARTNET,
  USER_ACTIVITY_LEDS_ARTNET_TIMEOUT,
  USER_ACTIVITY_LEDS_TEST,

  USER_ACTIVITY_DMX_INPUT,
  USER_ACTIVITY_DMX_OUTPUT,

  USER_ACTIVITY_MAX
};

enum user_alert {
  USER_ALERT_NONE = 0,

  USER_ALERT_ERROR_BOOT,
  USER_ALERT_ERROR_CONFIG,
  USER_ALERT_ERROR_SETUP,
  USER_ALERT_ERROR_WIFI,
  USER_ALERT_ERROR_START,
  USER_ALERT_ERROR_DMX,
  USER_ALERT_ERROR_LEDS_SEQUENCE,
  USER_ALERT_ERROR_LEDS_SEQUENCE_READ,
  USER_ALERT_ERROR_ATX_PSU_TIMEOUT,
  USER_ALERT_MAX
};

const char *user_power_str(enum user_power power);
const char *user_state_str(enum user_state state);
const char *user_activity_str(enum user_activity activity);
const char *user_alert_str(enum user_alert alert);

// distribute events
int init_user();

void user_power(enum user_power power);
void user_state(enum user_state state);
void user_activity(enum user_activity activity);
void user_alert(enum user_alert alert);

// user_buttons.c
void user_config_boot();
void user_config_press();
void user_config_hold();
void user_config_release();

void user_test_press();
void user_test_hold();
void user_test_release();
