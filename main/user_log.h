#pragma once

#include "user.h"

#include <freertos/FreeRTOS.h>

struct user_log {
  TickType_t tick;

  union {
    enum user_power power;
    enum user_state state;
    enum user_activity activity;
    enum user_alert alert;
  };
};

extern struct user_log user_power_log;
extern struct user_log user_state_log;
extern struct user_log user_activity_log;
extern struct user_log user_alert_log;

