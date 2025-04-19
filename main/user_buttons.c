#include "user.h"
#include "user_leds_output.h"

#include "config.h"
#include "console.h"
#include "leds_test.h"
#include "system.h"
#include "wifi.h"

#include <logging.h>

/* config */
void user_config_boot()
{
  LOG_WARN("disable config boot");

  disable_config();
}

void user_config_press()
{
  LOG_INFO("start config mode");

  override_user_led(USER_LED, USER_LEDS_PULSE);

  if (start_console() < 0) {
    user_alert(USER_ALERT_ERROR_START);
  }

  if (start_wifi_config()) {
    user_alert(USER_ALERT_ERROR_WIFI);
  }
}

void user_config_hold()
{
  LOG_WARN("reset config");

  user_state(USER_STATE_RESET);
  revert_user_led(USER_LED);

  reset_config();
  system_restart();
}

void user_config_release()
{
  revert_user_led(USER_LED);
}

/* test */
static bool user_test_held = false;

void user_test_press()
{
  user_test_held = false;

  LOG_INFO("trigger test mode");

  trigger_leds_test();
}

void user_test_hold()
{
  if (!user_test_held) {
    LOG_INFO("auto test mode");

    auto_leds_test();
  }

  user_test_held = true;
}

void user_test_release()
{
  if (user_test_held) {
    LOG_INFO("reset test mode");

    reset_leds_test();
  }

  user_test_held = false;
}
