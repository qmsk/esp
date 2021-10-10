#pragma once

#include <lwip/ip_addr.h>

enum user_state {
  // states
  USER_STATE_BOOT,
  USER_STATE_CONNECTING,
  USER_STATE_CONNECTED,
  USER_STATE_DISCONNECTED,
  USER_STATE_RESET,

  USER_STATE_MAX
};

enum user_activity {
  USER_ACTIVITY_SPI_LEDS,
  USER_ACTIVITY_DMX,

  USER_ACTIVITY_MAX
};

enum user_alert {
  USER_ALERT_ERROR_BOOT,
  USER_ALERT_ERROR_CONFIG,
  USER_ALERT_ERROR_SETUP,
  USER_ALERT_MAX
};

// distribute events
void user_state(enum user_state state);
void user_activity(enum user_activity activity);
void user_alert(enum user_alert alert);

// distribute config
void update_user_ipv4_address(ip4_addr_t ip_addr);

/*
 * Hard config reset + system restart.
 */
void user_reset();
