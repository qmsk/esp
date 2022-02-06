#include "status_leds.h"
#include "status_leds_set.h"

#include <sdkconfig.h>

#if CONFIG_STATUS_LEDS_USER_ENABLED
int status_leds_off_cmd(int argc, char **argv, void *ctx)
{
  set_user_led(STATUS_LED_OFF);

  return 0;
}

int status_leds_on_cmd(int argc, char **argv, void *ctx)
{
  set_user_led(STATUS_LED_ON);

  return 0;
}

int status_leds_slow_cmd(int argc, char **argv, void *ctx)
{
  set_user_led(STATUS_LED_SLOW);

  return 0;
}

int status_leds_fast_cmd(int argc, char **argv, void *ctx)
{
  set_user_led(STATUS_LED_FAST);

  return 0;
}
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
int status_leds_flash_cmd(int argc, char **argv, void *ctx)
{
  set_flash_led(STATUS_LED_FLASH);

  return 0;
}
#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
int status_leds_alert_cmd(int argc, char **argv, void *ctx)
{
  set_alert_led(STATUS_LED_ON);

  return 0;
}

int status_leds_clear_cmd(int argc, char **argv, void *ctx)
{
  set_alert_led(STATUS_LED_OFF);

  return 0;
}
#endif

const struct cmd status_leds_commands[] = {
#if CONFIG_STATUS_LEDS_USER_ENABLED
  { "off",    status_leds_off_cmd,      .describe = "Turn off USER LED" },
  { "on",     status_leds_on_cmd,       .describe = "Turn on USER LED" },
  { "slow",   status_leds_slow_cmd,     .describe = "Blink USER LED slowly" },
  { "fast",   status_leds_fast_cmd,     .describe = "Blink USER LED fast" },
#endif
#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  { "flash",  status_leds_flash_cmd,    .describe = "Blink FLASH LED once" },
#endif
#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  { "alert",  status_leds_alert_cmd,    .describe = "Turn on ALERT LED" },
  { "clear",  status_leds_clear_cmd,    .describe = "Turn off ALERT LED" },
#endif
  {}
};

const struct cmdtab status_leds_cmdtab = {
  .commands = status_leds_commands,
};
