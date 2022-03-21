#include "status_leds.h"
#include "status_leds_set.h"
#include "user.h"
#include "tasks.h"

#include <status_led.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

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
  [USER_STATE_DISCONNECTING]    = STATUS_LED_FAST,
  [USER_STATE_DISCONNECTED]     = STATUS_LED_SLOW,
  [USER_STATE_STOPPED]          = STATUS_LED_OFF,
  [USER_STATE_RESET]            = STATUS_LED_OFF,
};

enum status_led_mode user_alert_led_mode[USER_ALERT_MAX] = {
  [USER_ALERT_ERROR_BOOT]         = STATUS_LED_ON,
  [USER_ALERT_ERROR_CONFIG]       = STATUS_LED_SLOW,
  [USER_ALERT_ERROR_SETUP]        = STATUS_LED_FAST,
};

#if CONFIG_STATUS_LEDS_USER_ENABLED
static struct status_led *user_led;
xTaskHandle user_led_task;

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

static int start_user_led()
{
  struct task_options task_options = {
    .main       = status_led_main,
    .name       = STATUS_LEDS_USER_TASK_NAME,
    .stack_size = STATUS_LED_TASK_STACK,
    .arg        = user_led,
    .priority   = STATUS_LED_TASK_PRIORITY,
    .handle     = &user_led_task,
    .affinity   = STATUS_LED_TASK_AFFINITY,
  };

  if (start_task(task_options)) {
    LOG_ERROR("start_task");
    return -1;
  }

  return 0;
}

void set_user_led(enum status_led_mode mode)
{
  if (!user_led) {
    return;
  }

  if (status_led_set(user_led, mode)) {
    LOG_WARN("status_led_set");
  }
}
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
static struct status_led *flash_led;
xTaskHandle flash_led_task;

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

static int start_flash_led()
{
  struct task_options task_options = {
    .main       = status_led_main,
    .name       = STATUS_LEDS_FLASH_TASK_NAME,
    .stack_size = STATUS_LED_TASK_STACK,
    .arg        = flash_led,
    .priority   = STATUS_LED_TASK_PRIORITY,
    .handle     = &flash_led_task,
    .affinity   = STATUS_LED_TASK_AFFINITY,
  };

  if (start_task(task_options)) {
    LOG_ERROR("start_task");
    return -1;
  }

  return 0;
}

void set_flash_led(enum status_led_mode mode)
{
  if (!flash_led) {
    return;
  }

  if (status_led_set(flash_led, mode)) {
    LOG_WARN("status_led_set");
  }
}

#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
static struct status_led *alert_led;
xTaskHandle alert_led_task;

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

static int start_alert_led()
{
  struct task_options task_options = {
    .main       = status_led_main,
    .name       = STATUS_LEDS_ALERT_TASK_NAME,
    .stack_size = STATUS_LED_TASK_STACK,
    .arg        = alert_led,
    .priority   = STATUS_LED_TASK_PRIORITY,
    .handle     = &alert_led_task,
    .affinity   = STATUS_LED_TASK_AFFINITY,
  };

  if (start_task(task_options)) {
    LOG_ERROR("start_task");
    return -1;
  }

  return 0;
}

void set_alert_led(enum status_led_mode mode)
{
  // TODO: early gpio alert output before init_status_leds()?
  if (!alert_led) {
    return;
  }

  if (status_led_set(alert_led, mode)) {
    LOG_WARN("status_led_set");
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
    user_config_disable();
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
    status_leds_revert(status_led);
    user_config_reset();
  } else if (hold == 1) {
    // trigger once at start of flash sequence
    LOG_WARN("config");
    status_leds_override(status_led, STATUS_LED_PULSE);
    user_config_mode();
  } else if (released > 0) {
    LOG_WARN("cancel");
    status_leds_revert(status_led);
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
    status_leds_override(status_led, STATUS_LED_PULSE);
    user_test_mode();
  } else if (released) {
    LOG_WARN("cancel");
    status_leds_revert(status_led);
    user_test_cancel();
  }
}

#if CONFIG_IDF_TARGET_ESP32
// esp-idf vTaskDelayUntil compat macro is broken
# undef vTaskDelayUntil
# define vTaskDelayUntil xTaskDelayUntil
#endif

void status_leds_main(void *arg)
{
#if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG || CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  // needs flash LED task to be running
  init_config_button(flash_led);
#endif

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
  struct task_options task_options = {
    .main       = status_leds_main,
    .name       = STATUS_LEDS_TASK_NAME,
    .stack_size = STATUS_LEDS_TASK_STACK,
    .arg        = NULL,
    .priority   = STATUS_LEDS_TASK_PRIORITY,
    .handle     = &status_leds_task,
    .affinity   = STATUS_LEDS_TASK_AFFINITY,
  };
  int err;

#if CONFIG_STATUS_LEDS_USER_ENABLED
  if ((err = start_user_led())) {
    LOG_ERROR("start_user_led");
    return err;
  }
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  if ((err = start_flash_led())) {
    LOG_ERROR("start_flash_led");
    return err;
  }
#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  if ((err = start_alert_led())) {
    LOG_ERROR("start_alert_led");
    return err;
  }
#endif

  if (start_task(task_options)) {
    LOG_ERROR("start_task");
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

  (void) mode;

#if CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  set_flash_led(mode);
#endif
#if CONFIG_STATUS_LEDS_ALERT_MODE || CONFIG_STATUS_LEDS_ALERT_MODE_TEST
  set_alert_led(mode);
#endif
}
