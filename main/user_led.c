#include "user_led.h"

#include <status_led.h>
#include <logging.h>

static struct status_led *user_led;

#define USER_LED_GPIO      GPIO_NUM_16 // integrated LED on NodeMCU
#define USER_LED_INVERTED  true // active-low

enum status_led_mode user_event_led_mode[USER_EVENT_MAX] = {
  [USER_EVENT_BOOT]         = STATUS_LED_ON,
  [USER_EVENT_CONNECTING]   = STATUS_LED_FAST,
  [USER_EVENT_CONNECTED]    = STATUS_LED_SLOW,
  [USER_EVENT_DISCONNECTED] = STATUS_LED_OFF,
};

int init_user_led()
{
  const struct status_led_options user_led_options = {
    .gpio     = USER_LED_GPIO,
    .inverted = USER_LED_INVERTED,
  };
  const enum status_led_mode user_led_mode = user_event_led_mode[USER_EVENT_BOOT];

  if (status_led_new(&user_led, user_led_options, user_led_mode)) {
    LOG_ERROR("status_led_new");
    return -1;
  }

  return 0;
}

void user_led_event(enum user_event event)
{
  if (user_led && event < USER_EVENT_MAX) {
    if (status_led_mode(user_led, user_event_led_mode[event])) {
      LOG_WARN("status_led_mode");
    }
  }
}

// CLI
int set_user_led(enum status_led_mode mode)
{
  return status_led_mode(user_led, mode);
}

int user_led_off_cmd(int argc, char **argv, void *ctx)
{
  set_user_led(STATUS_LED_OFF);

  return 0;
}

int user_led_on_cmd(int argc, char **argv, void *ctx)
{
  set_user_led(STATUS_LED_ON);

  return 0;
}

int user_led_slow_cmd(int argc, char **argv, void *ctx)
{
  set_user_led(STATUS_LED_SLOW);

  return 0;
}

int user_led_fast_cmd(int argc, char **argv, void *ctx)
{
  set_user_led(STATUS_LED_FAST);

  return 0;
}

int user_led_flash_cmd(int argc, char **argv, void *ctx)
{
  set_user_led(STATUS_LED_FLASH);

  return 0;
}

const struct cmd user_led_commands[] = {
  { "off",    user_led_off_cmd,      .describe = "Turn off LED" },
  { "on",     user_led_on_cmd,       .describe = "Turn on LED" },
  { "slow",   user_led_slow_cmd,     .describe = "Blink LED slowly" },
  { "fast",   user_led_fast_cmd,     .describe = "Blink LED fast" },
  { "flash",  user_led_flash_cmd,    .describe = "Blink LED once" },
  {}
};

const struct cmdtab user_led_cmdtab = {
  .commands = user_led_commands,
};
