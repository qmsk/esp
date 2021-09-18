#pragma once

enum user_event {
  // states
  USER_EVENT_BOOT,
  USER_EVENT_CONNECTING,
  USER_EVENT_CONNECTED,
  USER_EVENT_DISCONNECTED,
  USER_EVENT_RESET,

  USER_EVENT_MAX,

  // events
  USER_EVENT_FLASH              = 0x10000,
  USER_EVENT_ACTIVITY_SPI_LEDS,
  USER_EVENT_ACTIVITY_DMX,

  USER_EVENT_ALERT          = 0x20000,
  USER_EVENT_ERROR,
};

// distribute events
void user_event(enum user_event event);

/*
 * Hard config reset + system restart.
 */
void user_reset();
