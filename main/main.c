#include "artnet.h"
#include "atx_psu.h"
#include "console.h"
#include "config.h"
#include "dev_mutex.h"
#include "dmx.h"
#include "eth.h"
#include "gpio.h"
#include "i2c_master.h"
#include "leds.h"
#include "log.h"
#include "http.h"
#include "pin_mutex.h"
#include "sdcard.h"
#include "system.h"
#include "user.h"
#include "usb_pd_sink.h"
#include "user_events.h"
#include "user_leds.h"
#include "wifi.h"

#include <logging.h>
#include <system.h>

void app_main(void)
{
  int err;

  // heap usage is likely to be lowest at app_main() start
  system_update_maximum_free_heap_size();

  if ((err = init_log())) {
    LOG_ERROR("init_log");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  LOG_INFO("boot");

  if ((err = init_pin_mutex())) {
    LOG_ERROR("init_pin_mutex");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = init_dev_mutex())) {
    LOG_ERROR("init_dev_mutex");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = init_gpio())) {
    LOG_ERROR("init_gpio");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

#if CONFIG_I2C_MASTER_ENABLED
  if ((err = init_i2c_master())) {
    LOG_ERROR("init_i2c_master");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }
#endif

#if CONFIG_I2C_GPIO_ENABLED
  if ((err = init_i2c_gpio())) {
    LOG_ERROR("init_i2c_gpio");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }
#endif

  if ((err = init_user())) {
    LOG_ERROR("init_user");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = init_user_leds())) {
    LOG_ERROR("init_user_leds");
    user_alert(USER_ALERT_ERROR_BOOT);
  }

  if ((err = init_user_events())) {
    LOG_ERROR("init_user_events");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = init_system())) {
    LOG_ERROR("init_system");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = init_console())) {
    LOG_ERROR("init_console");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

  if ((err = start_user_leds())) {
    LOG_ERROR("start_user_leds");
    user_alert(USER_ALERT_ERROR_BOOT);
  }

  if ((err = start_user_events())) {
    LOG_ERROR("start_user_events");
    user_alert(USER_ALERT_ERROR_BOOT);
    abort();
  }

#if CONFIG_SDCARD_ENABLED
  if ((err = init_sdcard())) {
    LOG_ERROR("init_sdcard");
    user_alert(USER_ALERT_ERROR_SETUP);
  }
#endif

#if CONFIG_USB_PD_SINK_ENABLED
  if ((err = init_usb_pd_sink())) {
    LOG_ERROR("init_usb_pd_sink");
    user_alert(USER_ALERT_ERROR_SETUP);
  }
#endif

  LOG_INFO("config");

  if ((err = init_config()) < 0) {
    LOG_ERROR("init_config");
    user_alert(USER_ALERT_ERROR_BOOT);
  } else if (err > 0) {
    LOG_WARN("init_config: not configured");
    user_alert(USER_ALERT_ERROR_CONFIG);
  }

  if ((err = start_console())) {
    LOG_ERROR("start_console");
    user_alert(USER_ALERT_ERROR_START);
  }

#if CONFIG_I2C_GPIO_ENABLED
  if ((err = start_i2c_gpio())) {
    LOG_ERROR("start_i2c_gpio");
    user_alert(USER_ALERT_ERROR_START);
  }
#endif

#if CONFIG_USB_PD_SINK_ENABLED
  if ((err = start_usb_pd_sink())) {
    LOG_ERROR("start_usb_pd_sink");
    user_alert(USER_ALERT_ERROR_START);
  }
#endif

  LOG_INFO("setup");

  if ((err = init_wifi())) {
    LOG_ERROR("init_wifi");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

#if CONFIG_ETH_ENABLED
  if ((err = init_eth())) {
    LOG_ERROR("init_eth");
    user_alert(USER_ALERT_ERROR_SETUP);
  }
#endif

  if ((err = init_http())) {
    LOG_ERROR("init_http");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if ((err = init_atx_psu())) {
    LOG_ERROR("init_atx_psu");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if ((err = init_artnet())) {
    LOG_ERROR("init_artnet");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if ((err = init_dmx())) {
    LOG_ERROR("init_dmx");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  if ((err = init_leds())) {
    LOG_ERROR("init_leds");
    user_alert(USER_ALERT_ERROR_SETUP);
  }

  LOG_INFO("start");

  if ((err = start_wifi_boot())) {
    LOG_ERROR("start_wifi_boot");
    user_alert(USER_ALERT_ERROR_START);
  }

#if CONFIG_ETH_ENABLED
  if ((err = start_eth())) {
    LOG_ERROR("start_eth");
    user_alert(USER_ALERT_ERROR_START);
  }
#endif

  if ((err = start_http())) {
    LOG_ERROR("start_http");
    user_alert(USER_ALERT_ERROR_START);
  }

  if ((err = start_atx_psu())) {
    LOG_ERROR("start_atx_psu");
    user_alert(USER_ALERT_ERROR_START);
  }

  if ((err = start_dmx())) {
    LOG_ERROR("start_dmx");
    user_alert(USER_ALERT_ERROR_START);
  }

  if ((err = start_leds())) {
    LOG_ERROR("start_leds");
    user_alert(USER_ALERT_ERROR_START);
  }

  // after inputs/outputs added
  if ((err = start_artnet())) {
    LOG_ERROR("start_artnet");
    user_alert(USER_ALERT_ERROR_START);
  }

  LOG_INFO("fini");
}
