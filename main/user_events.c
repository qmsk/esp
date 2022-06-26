#include <user_leds.h>
#include "user_leds.h"
#include "user_leds_config.h"
#include "user_leds_input.h"
#include "user_leds_output.h"

#include "tasks.h"

#include <logging.h>

// reset if held for >5s
#define USER_LEDS_CONFIG_RESET_THRESHOLD (5000 / portTICK_RATE_MS)

// reset if held for >1s
#define USER_LEDS_TEST_RESET_THRESHOLD (500 / portTICK_RATE_MS)

xTaskHandle user_events_task;

// TODO: read at boot -> disable config load, enable config mode
void init_user_config_input(enum user_led led)
{
  int ret;

  if ((ret = get_user_led(led)) < 0) {
    LOG_ERROR("get_user_led");
  } else if (ret) {
    LOG_WARN("config boot disabled");
    user_config_disable();
  }
}

void on_user_config_input(struct user_leds_input input, enum user_led led)
{
  static bool pressed = false;

  switch (input.event) {
    case USER_LEDS_INPUT_PRESS:
      // enter config mode
      LOG_WARN("config start");
      pressed = true;
      override_user_led(led, USER_LEDS_PULSE); // feedback
      user_config_mode();
      break;

    case USER_LEDS_INPUT_HOLD:
      if (!pressed) {
        // ignore when held at boot without press event
        LOG_WARN("config ignore");
      } else if (input.hold < USER_LEDS_CONFIG_RESET_THRESHOLD) {
        LOG_WARN("config hold");
      } else {
        LOG_WARN("config reset");
        user_state(USER_STATE_RESET);
        revert_user_led(led);
        user_config_reset();
      }
      break;

    case USER_LEDS_INPUT_RELEASE:
      LOG_WARN("cancel");
      pressed = false;
      revert_user_led(led);

      break;
  }
}

void on_user_test_input(struct user_leds_input input)
{
  switch (input.event) {
    case USER_LEDS_INPUT_PRESS:
      LOG_WARN("trigger");
      user_test_trigger();

      break;

    case USER_LEDS_INPUT_HOLD:
      if (input.hold > USER_LEDS_TEST_RESET_THRESHOLD) {
        LOG_WARN("hold");
        user_test_hold();
      } else {
        LOG_WARN("wait");
      }

      break;

    case USER_LEDS_INPUT_RELEASE:
      if (input.hold > USER_LEDS_TEST_RESET_THRESHOLD) {
        LOG_WARN("cancel");
        user_test_cancel();
      } else {
        LOG_WARN("coast");
      }

      break;
  }
}

void on_user_input(struct user_leds_input input)
{
  switch (input.index) {
  #if CONFIG_STATUS_LEDS_USER_MODE_TEST
    case USER_LED:
      on_user_test_input(input);
      break;
  #endif

  #if CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG || CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
    case FLASH_LED:
      on_user_config_input(input, ALERT_LED);
      break;
  #endif
  #if CONFIG_STATUS_LEDS_ALERT_MODE_TEST
    case ALERT_LED:
      on_user_test_input(input);
      break;
  #endif
  #if CONFIG_STATUS_LEDS_CONFIG_MODE
    case CONFIG_BUTTON:
      on_user_config_input(input, ALERT_LED);
      break;
  #endif
  #if CONFIG_STATUS_LEDS_TEST_MODE
    case TEST_BUTTON:
      on_user_test_input(input);
      break;
  #endif
  }
}

int init_user_events()
{
  return 0;
}

void user_events_main(void *arg)
{
  for (;;) {
    struct user_leds_input input;

    if (read_user_leds_input(&input, portMAX_DELAY)) {
      LOG_DEBUG("timeout");
      continue;
    }

    on_user_input(input);
  }
}

int start_user_events()
{
  struct task_options task_options = {
    .main       = user_events_main,
    .name       = USER_EVENTS_TASK_NAME,
    .stack_size = USER_EVENTS_TASK_STACK,
    .arg        = NULL,
    .priority   = USER_EVENTS_TASK_PRIORITY,
    .handle     = &user_events_task,
    .affinity   = USER_EVENTS_TASK_AFFINITY,
  };
  int err;

#if CONFIG_STATUS_LEDS_CONFIG_MODE
  init_user_config_input(CONFIG_BUTTON);
#elif CONFIG_STATUS_LEDS_FLASH_MODE_ACTIVITY_CONFIG || CONFIG_STATUS_LEDS_FLASH_MODE_ALERT_CONFIG
  init_user_config_input(FLASH_LED);
#endif

  if ((err = start_task(task_options))) {
    LOG_ERROR("start_task[%s]", USER_EVENTS_TASK_NAME);
    return err;
  }

  return 0;
}
