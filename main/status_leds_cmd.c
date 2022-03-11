#include "status_leds.h"
#include "status_leds_set.h"

#include <string.h>

#include <sdkconfig.h>

static int parse_status_led_mode(const char *str)
{
  if (!strcasecmp(str, "off")) {
    return STATUS_LED_OFF;
  } else if (!strcasecmp(str, "on")) {
    return STATUS_LED_ON;
  } else if (!strcasecmp(str, "slow")) {
    return STATUS_LED_SLOW;
  } else if (!strcasecmp(str, "fast")) {
    return STATUS_LED_FAST;
  } else if (!strcasecmp(str, "flash")) {
    return STATUS_LED_FLASH;
  } else if (!strcasecmp(str, "pulse")) {
    return STATUS_LED_PULSE;
  } else {
    return -1;
  }
}

#if CONFIG_STATUS_LEDS_USER_ENABLED
int status_leds_user_cmd(int argc, char **argv, void *ctx)
{
  enum status_led_mode mode = STATUS_LED_ON;
  const char *arg = NULL;
  int err;

  if (argc <= 1) {

  } else if ((err = cmd_arg_str(argc, argv, 1, &arg))) {
    return err;
  } else if ((mode = parse_status_led_mode(arg)) < 0) {
    return -CMD_ERR_ARGV;
  }

  set_user_led(mode);

  return 0;
}
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
int status_leds_flash_cmd(int argc, char **argv, void *ctx)
{
  enum status_led_mode mode = STATUS_LED_FLASH;
  const char *arg = NULL;
  int err;

  if (argc <= 1) {

  } else if ((err = cmd_arg_str(argc, argv, 1, &arg))) {
    return err;
  } else if ((mode = parse_status_led_mode(arg)) < 0) {
    return -CMD_ERR_ARGV;
  }

  set_flash_led(mode);

  return 0;
}
#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
int status_leds_alert_cmd(int argc, char **argv, void *ctx)
{
  enum status_led_mode mode = STATUS_LED_ON;
  const char *arg = NULL;
  int err;

  if (argc <= 1) {

  } else if ((err = cmd_arg_str(argc, argv, 1, &arg))) {
    return err;
  } else if ((mode = parse_status_led_mode(arg)) < 0) {
    return -CMD_ERR_ARGV;
  }

  set_alert_led(mode);

  return 0;
}
#endif

const struct cmd status_leds_commands[] = {
#if CONFIG_STATUS_LEDS_USER_ENABLED
  { "user",   status_leds_user_cmd,     .usage = "[MODE]",  .describe = "Set USER LED" },
#endif
#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  { "flash",  status_leds_flash_cmd,    .usage = "[MODE]",  .describe = "Set FLASH LED" },
#endif
#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  { "alert",  status_leds_alert_cmd,    .usage = "[MODE]",  .describe = "Set ALERT LED" },
#endif
  {}
};

const struct cmdtab status_leds_cmdtab = {
  .commands = status_leds_commands,
};
