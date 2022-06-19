#include "user_leds.h"
#include "user_leds_set.h"
#include "user.h"
#include "tasks.h"

#include <user_leds.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

// read FLASH every 1s
#define USER_LEDS_READ_TICKS (1000 / portTICK_RATE_MS)

// reset if held for >5s
#define USER_LEDS_CONFIG_RESET_THRESHOLD 5

// test if held for >1s
#define USER_LEDS_TEST_THRESHOLD 1

enum user_leds_state user_state_led_state[USER_STATE_MAX] = {
  [USER_STATE_BOOT]             = USER_LEDS_OFF,
  [USER_STATE_CONNECTING]       = USER_LEDS_FAST,
  [USER_STATE_CONNECTED]        = USER_LEDS_ON,
  [USER_STATE_DISCONNECTING]    = USER_LEDS_FAST,
  [USER_STATE_DISCONNECTED]     = USER_LEDS_SLOW,
  [USER_STATE_STOPPED]          = USER_LEDS_OFF,
  [USER_STATE_RESET]            = USER_LEDS_OFF,
};

enum user_leds_state user_alert_led_state[USER_ALERT_MAX] = {
  [USER_ALERT_ERROR_BOOT]         = USER_LEDS_ON,
  [USER_ALERT_ERROR_CONFIG]       = USER_LEDS_SLOW,
  [USER_ALERT_ERROR_SETUP]        = USER_LEDS_FAST,
};

// config
enum user_led {
#if CONFIG_STATUS_LEDS_USER_ENABLED
  USER_LED,
#endif
#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  FLASH_LED,
#endif
#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  ALERT_LED,
#endif

  USER_LEDS_COUNT
};

static const struct user_leds_options user_leds_options[] = {
#if CONFIG_STATUS_LEDS_USER_ENABLED
  [USER_LED] = {
    .gpio = CONFIG_STATUS_LEDS_USER_GPIO_NUM,
  #if CONFIG_STATUS_LEDS_USER_GPIO_INVERTED
    .mode = USER_LEDS_MODE_OUTPUT_BIT | USER_LEDS_MODE_INVERTED_BIT,
  #endif
  },
#endif
#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  [FLASH_LED] = {
    .gpio = CONFIG_STATUS_LEDS_FLASH_GPIO_NUM,
  #if CONFIG_STATUS_LEDS_FLASH_GPIO_INVERTED
    .mode = USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_OUTPUT_BIT | USER_LEDS_MODE_INVERTED_BIT,
  #endif
  },
#endif
#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  [ALERT_LED] = {
    .gpio = CONFIG_STATUS_LEDS_ALERT_GPIO_NUM,
  #if CONFIG_STATUS_LEDS_ALERT_GPIO_INVERTED
    .mode = USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_OUTPUT_BIT | USER_LEDS_MODE_INVERTED_BIT,
  #endif
  },
#endif
};

// state
struct user_leds *user_leds;
xTaskHandle user_leds_task;

int init_user_leds()
{
  int err;

  if ((err = user_leds_new(&user_leds, USER_LEDS_COUNT, user_leds_options))) {
    LOG_ERROR("user_leds_new");
    return err;
  }

  return 0;
}

static void init_config_button(struct user_leds *user_leds, enum user_led led)
{
  int ret;

  if ((ret = user_leds_read(user_leds, led)) < 0) {
    LOG_WARN("user_leds_read");
  } else if (ret) {
    LOG_WARN("config disable");

    // temporarily disable configuration loading
    user_config_disable();
  }
}

static void read_config_button(struct user_leds *user_leds, enum user_led led)
{
  static int boot = 1, hold = 0;
  int ret;
  int released = 0;

  if ((ret = user_leds_read(user_leds, led)) < 0) {
    LOG_WARN("user_leds_read");
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
  if (hold > USER_LEDS_CONFIG_RESET_THRESHOLD) {
    LOG_WARN("config reset");
    user_state(USER_STATE_RESET);
    user_leds_revert(user_leds, led);
    user_config_reset();
  } else if (hold == 1) {
    // trigger once at start of flash sequence
    LOG_WARN("config");
    user_leds_override(user_leds, led, USER_LEDS_PULSE);
    user_config_mode();
  } else if (released > 0) {
    LOG_WARN("cancel");
    user_leds_revert(user_leds, led);
  }
}

static void read_test_button(struct user_leds *user_leds, enum user_led led)
{
  static int hold = 0;
  int ret;
  int released = 0;

  if ((ret = user_leds_read(user_leds, led)) < 0) {
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
  if (hold > USER_LEDS_TEST_THRESHOLD) {
    LOG_WARN("continue");
  } else if (hold == USER_LEDS_TEST_THRESHOLD) {
    LOG_WARN("start");
    user_leds_override(user_leds, led, USER_LEDS_PULSE);
    user_test_mode();
  } else if (released) {
    LOG_WARN("cancel");
    user_leds_revert(user_leds, led);
    user_test_cancel();
  }
}

#if CONFIG_IDF_TARGET_ESP32
// esp-idf vTaskDelayUntil compat macro is broken
# undef vTaskDelayUntil
# define vTaskDelayUntil xTaskDelayUntil
#endif

// TODO: move to user_events.c
xTaskHandle user_events_task;

void user_events_main(void *arg)
{
  struct user_leds *user_leds = arg;

#if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG || CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  // needs flash LED task to be running
  init_config_button(user_leds, FLASH_LED);
#endif

  for (TickType_t tick = xTaskGetTickCount(); ; vTaskDelayUntil(&tick, USER_LEDS_READ_TICKS)) {
  #if CONFIG_STATUS_LEDS_USER_MODE_TEST
    read_test_button(user_leds, USER_LED);
  #endif
  #if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG || CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
    read_config_button(user_leds, FLASH_LED);
  #endif
  #if CONFIG_STATUS_LEDS_ALERT_MODE_TEST
    read_test_button(user_leds, ALERT_LED);
  #endif
  }
}

int start_user_leds()
{
  struct task_options leds_task_options = {
    .main       = user_leds_main,
    .name       = USER_LEDS_TASK_NAME,
    .stack_size = USER_LEDS_TASK_STACK,
    .arg        = user_leds,
    .priority   = USER_LEDS_TASK_PRIORITY,
    .handle     = &user_leds_task,
    .affinity   = USER_LEDS_TASK_AFFINITY,
  };
  struct task_options events_task_options = {
    .main       = user_events_main,
    .name       = USER_EVENTS_TASK_NAME,
    .stack_size = USER_EVENTS_TASK_STACK,
    .arg        = user_leds,
    .priority   = USER_EVENTS_TASK_PRIORITY,
    .handle     = &user_events_task,
    .affinity   = USER_EVENTS_TASK_AFFINITY,
  };
  int err;

  if (!user_leds) {
    LOG_ERROR("user_leds not initialized");
    return -1;
  }

  if ((err = start_task(leds_task_options))) {
    LOG_ERROR("start_task[%s]", USER_LEDS_TASK_NAME);
    return err;
  }

  if ((err = start_task(events_task_options))) {
    LOG_ERROR("start_task[%s]", USER_EVENTS_TASK_NAME);
    return err;
  }

  return 0;
}


#if CONFIG_STATUS_LEDS_USER_ENABLED
  int set_user_led(enum user_leds_state state, TickType_t timeout)
  {
    if (!user_leds) {
      return 0;
    }

    return user_leds_set(user_leds, USER_LED, state, timeout);
  }
#endif

#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  int set_flash_led(enum user_leds_state state, TickType_t timeout)
  {
    if (!user_leds) {
      return 0;
    }

    return user_leds_set(user_leds, FLASH_LED, state, timeout);
  }

#endif

#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  int set_alert_led(enum user_leds_state state, TickType_t timeout)
  {
    // TODO: early gpio alert output before init_user_leds()?
    if (!user_leds) {
      return 0;
    }

    return user_leds_set(user_leds, ALERT_LED, state, timeout);
  }
#endif

void user_leds_state(enum user_state state)
{
  enum user_leds_state leds_state = state < USER_STATE_MAX ? user_state_led_state[state] : USER_LEDS_ON;

#if CONFIG_STATUS_LEDS_USER_MODE || CONFIG_STATUS_LEDS_USER_MODE_TEST
  // blocking
  if (set_user_led(leds_state, portMAX_DELAY)) {
    LOG_WARN("set_user_led");
  }
#endif
}

void user_leds_activity(enum user_activity activity)
{
#if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG
  // this happens in the leds/dmx fastpath, do not block on the status_led mutex during each 1s interval 10ms status_led_read()
  if (set_flash_led(USER_LEDS_FLASH, 0)) {
    // just skip this FLASH activity
    LOG_DEBUG("set_flash_led");
  }
#endif
}

void user_leds_alert(enum user_alert alert)
{
  enum user_leds_state leds_state = alert < USER_ALERT_MAX ? user_alert_led_state[alert] : USER_LEDS_ON;

#if CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  // blocking
  if (set_flash_led(leds_state, portMAX_DELAY)) {
    LOG_WARN("set_flash_led");
  }
#elif CONFIG_STATUS_LEDS_ALERT_MODE || CONFIG_STATUS_LEDS_ALERT_MODE_TEST
  // blocking
  if (set_alert_led(leds_state, portMAX_DELAY)) {
    LOG_WARN("set_alert_led");
  }
#else
  (void) state;
#endif
}
