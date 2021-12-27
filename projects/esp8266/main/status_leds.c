#include "status_leds.h"
#include "config.h"

#include <status_led.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

#define STATUS_LEDS_TASK_STACK 1024
#define STATUS_LEDS_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static struct status_led *user_led;
static struct status_led *flash_led;
static struct status_led *alert_led;

static xTaskHandle status_leds_task;

// read FLASH every 1s
#define STATUS_LEDS_READ_TICKS (1000 / portTICK_RATE_MS)

// reset if held for >5s
#define STATUS_LEDS_FLASH_RESET_THRESHOLD 5

// test if held for >1s
#define STATUS_LEDS_ALERT_TEST_THRESHOLD 1

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
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
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
#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
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
#endif

static enum status_led_mode user_led_mode;

static void set_user_led(enum status_led_mode mode)
{
  user_led_mode = mode;

  if (!user_led) {
    return;
  } else if (status_led_mode(user_led, mode)) {
    LOG_WARN("status_led_mode");
  }
}

static void override_user_led(enum status_led_mode mode)
{
  if (!user_led) {
    return;
  } else if (status_led_mode(user_led, mode)) {
    LOG_WARN("status_led_mode");
  }
}

static void revert_user_led()
{
  if (!user_led) {
    return;
  } else if (status_led_mode(user_led, user_led_mode)) {
    LOG_WARN("status_led_mode");
  }
}

static void set_flash_led(enum status_led_mode mode)
{
  if (!flash_led) {
    return;
  } else if (status_led_mode(flash_led, mode)) {
    LOG_WARN("status_led_mode");
  }
}

static void set_alert_led(enum status_led_mode mode)
{
  if (!alert_led) {
    return;
  } else if (status_led_mode(alert_led, mode)) {
    LOG_WARN("status_led_mode");
  }
}

static int flash_boot = 1, flash_held = 0;

static void read_flash_led()
{
  int ret;
  int flash_released = 0;

  if ((ret = status_led_read(flash_led)) < 0) {
    LOG_WARN("status_led_read");
  } else if (ret && flash_boot) {
    // do not initiate config reset if held during boot, assume waking up after reset
    LOG_WARN("FLASH held, but still disarmed after boot");
  } else if (ret) {
    LOG_INFO("FLASH held");
    flash_held++;
  } else if (flash_held > 0) {
    LOG_INFO("FLASH released");
    flash_released = flash_held;
    flash_held = 0;
  } else if (flash_boot) {
    // reset boot state, accept press
    LOG_INFO("FLASH armed");
    flash_boot = 0;
  }

  // flash must be held for threshold samples to reset, and is cancled if released before that
  if (flash_held > STATUS_LEDS_FLASH_RESET_THRESHOLD) {
    LOG_WARN("request reset");
    user_state(USER_STATE_RESET);
    user_reset();
  } else if (flash_held == 1) {
    // trigger once at start of flash sequence
    LOG_WARN("request config");
    override_user_led(STATUS_LED_FAST);
    user_config();
  } else if (flash_released > 0) {
    LOG_WARN("cancel reset");
    revert_user_led();
  }

  flash_released = 0;
}

static int alert_held = 0;

static void read_alert_led()
{
  int ret;
  int alert_released = 0;

  if ((ret = status_led_read(alert_led)) < 0) {
    LOG_WARN("status_led_read");
  } else if (ret) {
    LOG_INFO("ALERT held=%d", alert_held);
    alert_held++;
  } else if (alert_held > 0) {
    LOG_INFO("ALERT released=%d", alert_held);
    alert_released = alert_held;
    alert_held = 0;
  }

  // alert must be held for threshold samples to trigger, and is held to keep the test going
  if (alert_held > STATUS_LEDS_ALERT_TEST_THRESHOLD) {
    LOG_WARN("continue test");
  } else if (alert_held == STATUS_LEDS_ALERT_TEST_THRESHOLD) {
    LOG_WARN("start test");
    user_test();
  } else if (alert_held == 1) {
    LOG_WARN("request test");
    override_user_led(STATUS_LED_SLOW);
  } else if (alert_released) {
    LOG_WARN("cancel test");
    revert_user_led();
    user_test_cancel();
  }
}

void status_leds_main(void *arg)
{
  for (TickType_t tick = xTaskGetTickCount(); ; vTaskDelayUntil(&tick, STATUS_LEDS_READ_TICKS)) {
    read_flash_led();

    if (alert_led) {
      read_alert_led();
    }
  }
}

static int init_status_leds_task()
{
  int ret;

  if ((ret = status_led_read(flash_led)) < 0) {
    LOG_WARN("status_led_read");
  } else if (ret) {
    LOG_WARN("boot with FLASH held");

    // temporarily disable configuration loading
    disable_config();
  }

  if (xTaskCreate(&status_leds_main, "status-leds", STATUS_LEDS_TASK_STACK, NULL, STATUS_LEDS_TASK_PRIORITY, &status_leds_task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  }

  return 0;
}

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

  if ((err = init_status_leds_task())) {
    LOG_ERROR("init_status_leds_task");
    return err;
  }

  return 0;
}

void status_leds_state(enum user_state state)
{
  if (state < USER_STATE_MAX) {
    set_user_led(user_state_led_mode[state]);
  }
}

void status_leds_activity(enum user_activity activity)
{
  set_flash_led(STATUS_LED_FLASH);
}

void status_leds_alert(enum user_alert alert)
{
  if (alert < USER_ALERT_MAX) {
    set_alert_led(user_alert_led_mode[alert]);
  } else {
    set_alert_led(STATUS_LED_ON);
  }
}

// CLI
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

int status_leds_flash_cmd(int argc, char **argv, void *ctx)
{
  set_flash_led(STATUS_LED_FLASH);

  return 0;
}

int status_leds_read_cmd(int argc, char **argv, void *ctx)
{
  read_flash_led();

  return 0;
}

int status_leds_alert_cmd(int argc, char **argv, void *ctx)
{
  set_alert_led(STATUS_LED_ON);

  return 0;
}

const struct cmd status_leds_commands[] = {
  { "off",    status_leds_off_cmd,      .describe = "Turn off USER LED" },
  { "on",     status_leds_on_cmd,       .describe = "Turn on USER LED" },
  { "slow",   status_leds_slow_cmd,     .describe = "Blink USER LED slowly" },
  { "fast",   status_leds_fast_cmd,     .describe = "Blink USER LED fast" },
  { "flash",  status_leds_flash_cmd,    .describe = "Blink FLASH LED once" },
  { "read",   status_leds_read_cmd,     .describe = "Read FLASH button" },
  { "alert",  status_leds_alert_cmd,    .describe = "Turn on ALERT LED" },
  {}
};

const struct cmdtab status_leds_cmdtab = {
  .commands = status_leds_commands,
};
