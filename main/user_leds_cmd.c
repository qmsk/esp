#include "user_leds.h"
#include "user_leds_output.h"

#include <logging.h>

#include <string.h>

#include <sdkconfig.h>

static int parse_user_leds_mode(const char *str)
{
  if (!strcasecmp(str, "off")) {
    return USER_LEDS_OFF;
  } else if (!strcasecmp(str, "on")) {
    return USER_LEDS_ON;
  } else if (!strcasecmp(str, "slow")) {
    return USER_LEDS_SLOW;
  } else if (!strcasecmp(str, "fast")) {
    return USER_LEDS_FAST;
  } else if (!strcasecmp(str, "flash")) {
    return USER_LEDS_FLASH;
  } else if (!strcasecmp(str, "pulse")) {
    return USER_LEDS_PULSE;
  } else {
    return -1;
  }
}

#if CONFIG_STATUS_LEDS_USER_ENABLED
int user_leds_user_cmd(int argc, char **argv, void *ctx)
{
  enum user_leds_state state = USER_LEDS_ON;
  const char *arg = NULL;
  int err;

  if (argc <= 1) {

  } else if ((err = cmd_arg_str(argc, argv, 1, &arg))) {
    return err;
  } else if ((state = parse_user_leds_mode(arg)) < 0) {
    return -CMD_ERR_ARGV;
  }

  if ((err = set_user_led(USER_LED, state))) {
    LOG_ERROR("set_user_led");
    return err;
  }

  return 0;
}
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
int user_leds_flash_cmd(int argc, char **argv, void *ctx)
{
  enum user_leds_state state = USER_LEDS_FLASH;
  const char *arg = NULL;
  int err;

  if (argc <= 1) {

  } else if ((err = cmd_arg_str(argc, argv, 1, &arg))) {
    return err;
  } else if ((state = parse_user_leds_mode(arg)) < 0) {
    return -CMD_ERR_ARGV;
  }

  if ((err = set_user_led(FLASH_LED, state))) {
    LOG_ERROR("set_user_led");
    return err;
  }

  return 0;
}
#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
int user_leds_alert_cmd(int argc, char **argv, void *ctx)
{
  enum user_leds_state state = USER_LEDS_ON;
  const char *arg = NULL;
  int err;

  if (argc <= 1) {

  } else if ((err = cmd_arg_str(argc, argv, 1, &arg))) {
    return err;
  } else if ((state = parse_user_leds_mode(arg)) < 0) {
    return -CMD_ERR_ARGV;
  }

  if ((err = set_user_led(ALERT_LED, state))) {
    LOG_ERROR("set_alert_led");
    return err;
  }

  return 0;
}
#endif

const struct cmd user_leds_commands[] = {
#if CONFIG_STATUS_LEDS_USER_ENABLED
  { "user",   user_leds_user_cmd,     .usage = "[STATE]", .describe = "Set USER LED" },
#endif
#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  { "flash",  user_leds_flash_cmd,    .usage = "[STATE]", .describe = "Set FLASH LED" },
#endif
#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  { "alert",  user_leds_alert_cmd,    .usage = "[STATE]", .describe = "Set ALERT LED" },
#endif
  {}
};

const struct cmdtab user_leds_cmdtab = {
  .commands = user_leds_commands,
};
