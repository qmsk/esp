#pragma once

#define CONSOLE_CLI_ENABLED true
#define CONSOLE_CLI_TIMEOUT 0 // disabled

struct console_config {
  bool enabled;
  uint16_t timeout;
};

extern struct console_config console_config;
