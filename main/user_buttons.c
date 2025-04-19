#include "user.h"
#include "user_leds.h"

#include <logging.h>

void user_config_disable()
{
  disable_config();
}

void user_config_mode()
{
  if (start_console() < 0) {
    user_alert(USER_ALERT_ERROR_START);
  }

  if (start_wifi_config()) {
    user_alert(USER_ALERT_ERROR_WIFI);
  }
}

void user_config_reset()
{
  reset_config();
  system_restart();
}

void user_test_trigger()
{
  trigger_leds_test();
}

void user_test_hold()
{
  auto_leds_test();
}

void user_test_cancel()
{
  reset_leds_test();
}
