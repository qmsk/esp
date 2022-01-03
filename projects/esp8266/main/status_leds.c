#include "status_leds.h"
#include "config.h"

#include <status_led.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

#define STATUS_LEDS_TASK_STACK 1024
#define STATUS_LEDS_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static xTaskHandle status_leds_task;

// read FLASH every 1s
#define STATUS_LEDS_READ_TICKS (1000 / portTICK_RATE_MS)

// reset if held for >5s
#define STATUS_LEDS_CONFIG_RESET_THRESHOLD 5

// test if held for >1s
#define STATUS_LEDS_TEST_THRESHOLD 1

enum status_led_mode user_state_led_mode[USER_STATE_MAX] = {
  [USER_STATE_BOOT]             = STATUS_LED_OFF,
  [USER_STATE_CONNECTING]       = STATUS_LED_FAST,
  [USER_STATE_CONNECTED]        = STATUS_LED_ON,
  [USER_STATE_DISCONNECTED]     = STATUS_LED_SLOW,
  [USER_STATE_RESET]            = STATUS_LED_OFF,
};

enum status_led_mode user_alert_led_mode[USER_ALERT_MAX] = {
  [USER_ALERT_ERROR_BOOT]         = STATUS_LED_ON,
  [USER_ALERT_ERROR_CONFIG]       = STATUS_LED_SLOW,
  [USER_ALERT_ERROR_SETUP]        = STATUS_LED_FAST,
};

#if CONFIG_STATUS_LEDS_USER_ENABLED
static struct status_led *user_led;

static int init_user_led()
{
  const struct status_led_options led_options = {
    .gpio     = CONFIG_STATUS_LEDS_USER_GPIO_NUM,

  #if CONFIG_STATUS_LEDS_USER_GPIO_INVERTED
    .inverted = true,
  #endif
  };
  const enum status_led_mode led_mode = user_state_led_mode[USER_STATE_BOOT];

  LOG_INFO("gpio=%u inverted=%d", led_options.gpio, led_options.inverted);

  if (status_led_new(&user_led, led_options, led_mode)) {
    LOG_ERROR("status_led_new");
    return -1;
  }

  return 0;
}

static enum status_led_mode user_led_mode;

static void set_user_led(enum status_led_mode mode)
{
  user_led_mode = mode;

  if (!user_led) {
    return;
  }

  if (status_led_mode(user_led, mode)) {
    LOG_WARN("status_led_mode");
  }
}

static void override_user_led(enum status_led_mode mode)
{
  if (!user_led) {
    return;
  }

  if (status_led_mode(user_led, mode)) {
    LOG_WARN("status_led_mode");
  }
}

static void revert_user_led()
{
  if (!user_led) {
    return;
  }

  if (status_led_mode(user_led, user_led_mode)) {
    LOG_WARN("status_led_mode");
  }
}
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
static struct status_led *flash_led;

static int init_flash_led()
{
  const struct status_led_options led_options = {
    .gpio     = CONFIG_STATUS_LEDS_FLASH_GPIO_NUM,

  #if CONFIG_STATUS_LEDS_FLASH_GPIO_INVERTED
    .inverted = true,
  #endif
  };
  const enum status_led_mode led_mode = STATUS_LED_OFF;

  LOG_INFO("gpio=%u inverted=%d", led_options.gpio, led_options.inverted);

  if (status_led_new(&flash_led, led_options, led_mode)) {
    LOG_ERROR("status_led_new");
    return -1;
  }

  return 0;
}

static void set_flash_led(enum status_led_mode mode)
{
  if (!flash_led) {
    return;
  }

  if (status_led_mode(flash_led, mode)) {
    LOG_WARN("status_led_mode");
  }
}

#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
static struct status_led *alert_led;

static int init_alert_led()
{
  const struct status_led_options led_options = {
    .gpio     = CONFIG_STATUS_LEDS_ALERT_GPIO_NUM,

  #if CONFIG_STATUS_LEDS_ALERT_GPIO_INVERTED
    .inverted = true,
  #endif
  };
  const enum status_led_mode led_mode = STATUS_LED_OFF;

  LOG_INFO("gpio=%u inverted=%d", led_options.gpio, led_options.inverted);

  if (status_led_new(&alert_led, led_options, led_mode)) {
    LOG_ERROR("status_led_new");
    return -1;
  }

  return 0;
}

static void set_alert_led(enum status_led_mode mode)
{
  // TODO: early gpio alert output before init_status_leds()?
  if (!alert_led) {
    return;
  }

  if (status_led_mode(alert_led, mode)) {
    LOG_WARN("status_led_mode");
  }
}
#endif

int init_status_leds()
{
  int err;

#if CONFIG_STATUS_LEDS_USER_ENABLED
  if ((err = init_user_led())) {
    LOG_ERROR("init_user_led");
    return err;
  }
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  if ((err = init_flash_led())) {
    LOG_ERROR("init_flash_led");
    return err;
  }
#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  if ((err = init_alert_led())) {
    LOG_ERROR("init_alert_led");
    return err;
  }
#endif

  return 0;
}

static void init_config_button(struct status_led *status_led)
{
  int ret;

  if ((ret = status_led_read(status_led)) < 0) {
    LOG_WARN("status_led_read");
  } else if (ret) {
    LOG_WARN("config disable");

    // temporarily disable configuration loading
    disable_config();
  }
}

static void read_config_button(struct status_led *status_led)
{
  static int boot = 1, hold = 0;
  int ret;
  int released = 0;

  if ((ret = status_led_read(status_led)) < 0) {
    LOG_WARN("status_led_read");
    return;
  } else if (ret && boot) {
    // do not initiate config reset if held during boot, assume waking up after reset
    LOG_WARN("ignored (disarmed at boot)");
  } else if (ret) {
    hold++;
    LOG_INFO("hold=%d", hold);
  } else if (hold > 0) {
    released = hold;
    LOG_INFO("released=%d", released);
    hold = 0;
  } else if (boot) {
    // reset boot state, accept button press
    LOG_INFO("armed");
    boot = 0;
  }

  // button must be held for threshold samples to reset, and is canceled if released before that
  if (hold > STATUS_LEDS_CONFIG_RESET_THRESHOLD) {
    LOG_WARN("config reset");
    user_state(USER_STATE_RESET);
    user_config_reset();
  } else if (hold == 1) {
    // trigger once at start of flash sequence
    LOG_WARN("config");
    override_user_led(STATUS_LED_FAST);
    user_config();
  } else if (released > 0) {
    LOG_WARN("cancel");
    revert_user_led();
  }
}

static void read_test_button(struct status_led *status_led)
{
  static int hold = 0;
  int ret;
  int released = 0;

  if ((ret = status_led_read(status_led)) < 0) {
    LOG_WARN("status_led_read");
    return;
  } else if (ret) {
    hold++;
    LOG_INFO("hold=%d", hold);
  } else if (hold > 0) {
    released = hold;
    LOG_INFO("released=%d", released);
    hold = 0;
  }

  // button must be held for threshold samples to trigger, and is held to keep the test active
  if (hold > STATUS_LEDS_TEST_THRESHOLD) {
    LOG_WARN("continue");
  } else if (hold == STATUS_LEDS_TEST_THRESHOLD) {
    LOG_WARN("start");
    override_user_led(STATUS_LED_SLOW);
    user_test();
  } else if (released) {
    LOG_WARN("cancel");
    revert_user_led();
    user_test_cancel();
  }
}

void status_leds_main(void *arg)
{
  for (TickType_t tick = xTaskGetTickCount(); ; vTaskDelayUntil(&tick, STATUS_LEDS_READ_TICKS)) {
  #if CONFIG_STATUS_LEDS_USER_MODE_TEST
    read_test_button(user_led);
  #endif
  #if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG || CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
    read_config_button(flash_led);
  #endif
  #if CONFIG_STATUS_LEDS_ALERT_MODE_TEST
    read_test_button(alert_led);
  #endif
  }
}

int start_status_leds()
{
#if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG || CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  init_config_button(flash_led);
#endif

  if (xTaskCreate(&status_leds_main, "status-leds", STATUS_LEDS_TASK_STACK, NULL, STATUS_LEDS_TASK_PRIORITY, &status_leds_task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  }

  return 0;
}

void status_leds_state(enum user_state state)
{
  enum status_led_mode mode = state < USER_STATE_MAX ? user_state_led_mode[state] : STATUS_LED_ON;

#if CONFIG_STATUS_LEDS_USER_MODE || CONFIG_STATUS_LEDS_USER_MODE_TEST
  set_user_led(mode);
#endif
}

void status_leds_activity(enum user_activity activity)
{
#if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG
  set_flash_led(STATUS_LED_FLASH);
#endif
}

void status_leds_alert(enum user_alert alert)
{
  enum status_led_mode mode = alert < USER_ALERT_MAX ? user_alert_led_mode[alert] : STATUS_LED_ON;

#if CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  set_flash_led(mode);
#endif
#if CONFIG_STATUS_LEDS_ALERT_MODE || CONFIG_STATUS_LEDS_ALERT_MODE_TEST
  set_alert_led(mode);
#endif
}

// CLI
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
