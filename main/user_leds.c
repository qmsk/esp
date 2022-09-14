#include "user_leds.h"
#include "user_leds_config.h"
#include "user_leds_input.h"
#include "user_leds_output.h"
#include "user.h"
#include "i2c_gpio.h"
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
#if GPIO_I2C_ENABLED
  #define USER_LEDS_GPIO_I2C_TIMEOUT (20 / portTICK_RATE_MS)
#endif

#define USER_LED_MODE_OUTPUT_BITS (USER_LEDS_MODE_OUTPUT_BIT)
#if CONFIG_STATUS_LEDS_USER_MODE_TEST
  #define USER_LED_MODE_INPUT_BITS (USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_INTERRUPT_BIT)
#else
  #define USER_LED_MODE_INPUT_BITS 0
#endif
#if CONFIG_STATUS_LEDS_USER_GPIO_INVERTED
  #define USER_LED_MODE_INVERTED_BITS (USER_LEDS_MODE_INVERTED_BIT)
#else
  #define USER_LED_MODE_INVERTED_BITS 0
#endif

#define FLASH_LED_MODE_OUTPUT_BITS (USER_LEDS_MODE_OUTPUT_BIT)
#if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG || CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  #define FLASH_LED_MODE_INPUT_BITS (USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_INTERRUPT_BIT)
#else
  #define FLASH_LED_MODE_INPUT_BITS 0
#endif
#if CONFIG_STATUS_LEDS_FLASH_GPIO_INVERTED
  #define FLASH_LED_MODE_INVERTED_BITS (USER_LEDS_MODE_INVERTED_BIT)
#else
  #define FLASH_LED_MODE_INVERTED_BITS 0
#endif

#define ALERT_LED_MODE_OUTPUT_BITS (USER_LEDS_MODE_OUTPUT_BIT)
#if CONFIG_STATUS_LEDS_ALERT_MODE_TEST
  #define ALERT_LED_MODE_INPUT_BITS (USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_INTERRUPT_BIT)
#else
  #define ALERT_LED_MODE_INPUT_BITS 0
#endif
#if CONFIG_STATUS_LEDS_ALERT_GPIO_INVERTED
  #define ALERT_LED_MODE_INVERTED_BITS (USER_LEDS_MODE_INVERTED_BIT)
#else
  #define ALERT_LED_MODE_INVERTED_BITS 0
#endif

#if CONFIG_STATUS_LEDS_CONFIG_ENABLED
  #define CONFIG_BUTTON_MODE_INPUT_BITS (USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_INTERRUPT_BIT)
#endif
#if CONFIG_STATUS_LEDS_CONFIG_GPIO_INVERTED
  #define CONFIG_BUTTON_MODE_INVERTED_BITS (USER_LEDS_MODE_INVERTED_BIT)
#else
  #define CONFIG_BUTTON_MODE_INVERTED_BITS 0
#endif

#if CONFIG_STATUS_LEDS_TEST_ENABLED
  #define TEST_BUTTON_MODE_INPUT_BITS (USER_LEDS_MODE_INPUT_BIT | USER_LEDS_MODE_INTERRUPT_BIT)
#endif
#if CONFIG_STATUS_LEDS_TEST_GPIO_INVERTED
  #define TEST_BUTTON_MODE_INVERTED_BITS (USER_LEDS_MODE_INVERTED_BIT)
#else
  #define TEST_BUTTON_MODE_INVERTED_BITS 0
#endif

static struct gpio_options user_leds_gpio = {
#if CONFIG_STATUS_LEDS_GPIO_TYPE_HOST
  .type = GPIO_TYPE_HOST,
#elif CONFIG_STATUS_LEDS_GPIO_TYPE_I2C_GPIO_0
  .type = GPIO_TYPE_I2C,
  .i2c_timeout = USER_LEDS_GPIO_I2C_TIMEOUT,
#else
  #error "Invalid STATUS_LEDS_GPIO_TYPE"
#endif
};

#if CONFIG_STATUS_LEDS_GPIO_TYPE_I2C_GPIO_0
  #define USER_LEDS_I2C_GPIO_DEV_ID 0
  #define USER_LEDS_I2C_GPIO_DEV I2C_GPIO_DEV(0)
#endif

static struct user_leds_options user_leds_options[USER_LEDS_COUNT] = {
#if CONFIG_STATUS_LEDS_USER_ENABLED
  [USER_LED] = {
    .gpio_pin = CONFIG_STATUS_LEDS_USER_GPIO_NUM,
    .mode     = USER_LED_MODE_INPUT_BITS | USER_LED_MODE_OUTPUT_BITS | USER_LED_MODE_INVERTED_BITS,
  },
#endif
#if CONFIG_STATUS_LEDS_FLASH_ENABLED
  [FLASH_LED] = {
    .gpio_pin = CONFIG_STATUS_LEDS_FLASH_GPIO_NUM,
    .mode     = FLASH_LED_MODE_INPUT_BITS | FLASH_LED_MODE_OUTPUT_BITS | FLASH_LED_MODE_INVERTED_BITS,
  },
#endif
#if CONFIG_STATUS_LEDS_ALERT_ENABLED
  [ALERT_LED] = {
    .gpio_pin = CONFIG_STATUS_LEDS_ALERT_GPIO_NUM,
    .mode     = ALERT_LED_MODE_INPUT_BITS | ALERT_LED_MODE_OUTPUT_BITS | ALERT_LED_MODE_INVERTED_BITS,
  },
#endif
#if CONFIG_STATUS_LEDS_CONFIG_ENABLED
  [CONFIG_BUTTON] = {
    .gpio_pin = CONFIG_STATUS_LEDS_CONFIG_GPIO_NUM,
    .mode     = CONFIG_BUTTON_MODE_INPUT_BITS | CONFIG_BUTTON_MODE_INVERTED_BITS,
  },
#endif
#if CONFIG_STATUS_LEDS_TEST_ENABLED
  [TEST_BUTTON] = {
    .gpio_pin = CONFIG_STATUS_LEDS_TEST_GPIO_NUM,
    .mode     = TEST_BUTTON_MODE_INPUT_BITS | TEST_BUTTON_MODE_INVERTED_BITS,
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

  switch(user_leds_gpio.type) {
    case GPIO_TYPE_HOST:
      LOG_INFO("host gpio");
      break;

  #if GPIO_I2C_ENABLED
    case GPIO_TYPE_I2C:
    #ifdef USER_LEDS_I2C_GPIO_DEV
      user_leds_gpio.i2c_dev = USER_LEDS_I2C_GPIO_DEV;

      LOG_INFO("i2c-gpio%d: timeout=%d", USER_LEDS_I2C_GPIO_DEV_ID,
        user_leds_gpio.i2c_timeout
      );
      break;
    #endif
  #endif
  }

  if ((err = user_leds_new(&user_leds, &user_leds_gpio, USER_LEDS_COUNT, user_leds_options))) {
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
  if (!user_leds) {
    return 0;
  }

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
  if (!user_leds) {
    return 0;
  }

  user_leds_override[led] = true;

  return user_leds_set(user_leds, led, state);
}

int revert_user_led(enum user_led led)
{
  enum user_leds_state state = user_leds_state[led];

  if (!user_leds) {
    return 0;
  }

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
#if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY || CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG
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
  (void) leds_state;
#endif
}
