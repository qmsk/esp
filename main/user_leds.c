#include "user_leds.h"
#include "user_leds_config.h"
#include "user_leds_input.h"
#include "user_leds_output.h"
#include "user.h"
#include "tasks.h"

#include <user_leds.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

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

// state
struct user_leds *user_leds;
xTaskHandle user_leds_task;

// revert/override
enum user_leds_state user_leds_state[USER_LEDS_COUNT];
bool user_leds_override[USER_LEDS_COUNT];

QueueHandle_t user_leds_input_queue;

// config
#define USER_LED_MODE_OUTPUT_BIT USER_LEDS_MODE_OUTPUT_BIT
#if CONFIG_STATUS_LEDS_USER_MODE_TEST
  #define USER_LED_MODE_INPUT_BIT USER_LEDS_MODE_INPUT_BIT
#else
  #define USER_LED_MODE_INPUT_BIT 0
#endif
#if CONFIG_STATUS_LEDS_USER_GPIO_INVERTED
  #define USER_LED_MODE_INVERTED_BIT USER_LEDS_MODE_INVERTED_BIT
#else
  #define USER_LED_MODE_INVERTED_BIT 0
#endif

#define FLASH_LED_MODE_OUTPUT_BIT USER_LEDS_MODE_OUTPUT_BIT
#if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG || CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  #define FLASH_LED_MODE_INPUT_BIT USER_LEDS_MODE_INPUT_BIT
#else
  #define FLASH_LED_MODE_INPUT_BIT 0
#endif
#if CONFIG_STATUS_LEDS_FLASH_GPIO_INVERTED
  #define FLASH_LED_MODE_INVERTED_BIT USER_LEDS_MODE_INVERTED_BIT
#else
  #define FLASH_LED_MODE_INVERTED_BIT 0
#endif

#define ALERT_LED_MODE_OUTPUT_BIT USER_LEDS_MODE_OUTPUT_BIT
#if CONFIG_STATUS_LEDS_ALERT_MODE_TEST
  #define ALERT_LED_MODE_INPUT_BIT USER_LEDS_MODE_INPUT_BIT
#else
  #define ALERT_LED_MODE_INPUT_BIT 0
#endif
#if CONFIG_STATUS_LEDS_ALERT_GPIO_INVERTED
  #define ALERT_LED_MODE_INVERTED_BIT USER_LEDS_MODE_INVERTED_BIT
#else
  #define ALERT_LED_MODE_INVERTED_BIT 0
#endif

static struct user_leds_options user_leds_options[] = {
#if CONFIG_STATUS_LEDS_USER_ENABLED
  [USER_LED] = {
    .gpio = CONFIG_STATUS_LEDS_USER_GPIO_NUM,
    .mode = USER_LED_MODE_INPUT_BIT | USER_LED_MODE_OUTPUT_BIT | USER_LED_MODE_INVERTED_BIT,
  },
#endif
#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  [FLASH_LED] = {
    .gpio = CONFIG_STATUS_LEDS_FLASH_GPIO_NUM,
    .mode = FLASH_LED_MODE_INPUT_BIT | FLASH_LED_MODE_OUTPUT_BIT | FLASH_LED_MODE_INVERTED_BIT,
  },
#endif
#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  [ALERT_LED] = {
    .gpio = CONFIG_STATUS_LEDS_ALERT_GPIO_NUM,
    .mode = ALERT_LED_MODE_INPUT_BIT | ALERT_LED_MODE_OUTPUT_BIT | ALERT_LED_MODE_INVERTED_BIT,
  },
#endif
};

int init_user_leds()
{
  int err;

  if ((user_leds_input_queue = xQueueCreate(1, sizeof(struct user_leds_input))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  for (unsigned i = 0; i < USER_LEDS_COUNT; i++) {
    user_leds_options[i].input_queue = user_leds_input_queue;
  }

  if ((err = user_leds_new(&user_leds, USER_LEDS_COUNT, user_leds_options))) {
    LOG_ERROR("user_leds_new");
    return err;
  }

  return 0;
}

int start_user_leds()
{
  struct task_options task_options = {
    .main       = user_leds_main,
    .name       = USER_LEDS_TASK_NAME,
    .stack_size = USER_LEDS_TASK_STACK,
    .arg        = user_leds,
    .priority   = USER_LEDS_TASK_PRIORITY,
    .handle     = &user_leds_task,
    .affinity   = USER_LEDS_TASK_AFFINITY,
  };
  int err;

  if (!user_leds) {
    LOG_ERROR("user_leds not initialized");
    return -1;
  }

  if ((err = start_task(task_options))) {
    LOG_ERROR("start_task[%s]", USER_LEDS_TASK_NAME);
    return err;
  }

  return 0;
}

int get_user_led(enum user_led led)
{
  return user_leds_get(user_leds, led);
}

int read_user_leds_input(struct user_leds_input *inputp, TickType_t timeout)
{
  if (!xQueueReceive(user_leds_input_queue, inputp, timeout)) {
    return 1;
  } else {
    return 0;
  }
}

int override_user_led(enum user_led led, enum user_leds_state state)
{
  user_leds_override[led] = true;

  return user_leds_set(user_leds, led, state);
}

int revert_user_led(enum user_led led)
{
  enum user_leds_state state = user_leds_state[led];

  user_leds_override[led] = false;

  return user_leds_set(user_leds, led, state);
}

int set_user_led(enum user_led led, enum user_leds_state state)
{
  if (!user_leds) {
    return 0;
  }

  user_leds_state[led] = state;

  if (!user_leds_override[led]) {
    return user_leds_set(user_leds, led, state);
  } else {
    return 0;
  }
}

void set_user_leds_state(enum user_state state)
{
  enum user_leds_state leds_state = state < USER_STATE_MAX ? user_state_led_state[state] : USER_LEDS_ON;

#if CONFIG_STATUS_LEDS_USER_MODE || CONFIG_STATUS_LEDS_USER_MODE_TEST
  // blocking
  if (set_user_led(USER_LED, leds_state)) {
    LOG_WARN("set_user_led");
  }
#endif
}

void set_user_leds_activity(enum user_activity activity)
{
#if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG
  if (set_user_led(FLASH_LED, USER_LEDS_FLASH)) {
    // just skip this FLASH activity
    LOG_DEBUG("set_user_led");
  }
#endif
}

void set_user_leds_alert(enum user_alert alert)
{
  enum user_leds_state leds_state = alert < USER_ALERT_MAX ? user_alert_led_state[alert] : USER_LEDS_ON;

#if CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  // blocking
  if (set_user_led(FLASH_LED, leds_state)) {
    LOG_WARN("set_flash_led");
  }
#elif CONFIG_STATUS_LEDS_ALERT_MODE || CONFIG_STATUS_LEDS_ALERT_MODE_TEST
  // blocking
  if (set_user_led(ALERT_LED, leds_state)) {
    LOG_WARN("set_user_led");
  }
#else
  (void) state;
#endif
}
