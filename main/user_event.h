#pragma once

enum user_event {
  USER_EVENT_BOOT,
  USER_EVENT_CONNECTING,
  USER_EVENT_CONNECTED,
  USER_EVENT_DISCONNECTED,

  USER_EVENT_MAX
};

// distribute events
void user_event(enum user_event event);
