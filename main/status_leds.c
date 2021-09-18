#include "status_leds.h"

#include <status_led.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define STATUS_LEDS_TASK_STACK 1024
#define STATUS_LEDS_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static struct status_led *user_led;
static struct status_led *flash_led;
static struct status_led *alert_led;

static xTaskHandle status_leds_task;

// read FLASH every 1s, reset if held for >5s
#define STATUS_LEDS_FLASH_READ_PERIOD 1000
#define STATUS_LEDS_FLASH_RESET_THRESHOLD 5

#define USER_LED
#define USER_LED_GPIO      GPIO_NUM_16  // integrated LED on NodeMCU
#define USER_LED_INVERTED  true         // active-low will pull-up

#define FLASH_LED
#define FLASH_LED_GPIO      GPIO_NUM_0  // integrated button on NodeMCU
#define FLASH_LED_INVERTED  true        // active-low with pull-up

#define ALERT_LED
#define ALERT_LED_GPIO      GPIO_NUM_15 // XXX: shared with spi_master CS
#define ALERT_LED_INVERTED  false       // active-high with pull-down

enum status_led_mode user_event_led_mode[USER_EVENT_MAX] = {
  [USER_EVENT_BOOT]             = STATUS_LED_ON,
  [USER_EVENT_CONNECTING]       = STATUS_LED_FAST,
  [USER_EVENT_CONNECTED]        = STATUS_LED_OFF,
  [USER_EVENT_DISCONNECTED]     = STATUS_LED_SLOW,

  [USER_EVENT_RESET_REQUESTED]  = STATUS_LED_FAST,
  [USER_EVENT_RESET_CONFIRMED]  = STATUS_LED_ON,
  [USER_EVENT_RESET_CANCELED]   = STATUS_LED_OFF,
};

#ifdef USER_LED
static int init_user_led()
{
  const struct status_led_options led_options = {
    .gpio     = USER_LED_GPIO,
    .inverted = USER_LED_INVERTED,
  };
  const enum status_led_mode led_mode = user_event_led_mode[USER_EVENT_BOOT];

  LOG_INFO("gpio=%u inverted=%d", led_options.gpio, led_options.inverted);

  if (status_led_new(&user_led, led_options, led_mode)) {
    LOG_ERROR("status_led_new");
    return -1;
  }

  return 0;
}
#endif

#ifdef FLASH_LED
static int init_flash_led()
{
  const struct status_led_options led_options = {
    .gpio     = FLASH_LED_GPIO,
    .inverted = FLASH_LED_INVERTED,
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

#ifdef ALERT_LED
static int init_alert_led()
{
  const struct status_led_options led_options = {
    .gpio     = ALERT_LED_GPIO,
    .inverted = ALERT_LED_INVERTED,
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

static void status_leds_main(void *arg)
{
  static int flash_held = 0, flash_released = 0;
  int ret;

  for (TickType_t tick = xTaskGetTickCount(); ; vTaskDelayUntil(&tick, STATUS_LEDS_FLASH_READ_PERIOD / portTICK_RATE_MS)) {
    if ((ret = status_led_read(flash_led)) < 0) {
      LOG_WARN("status_led_read");
    } else if (ret) {
      flash_held++;
    } else if (flash_held > 0) {
      flash_released = flash_held;
      flash_held = 0;
    }

    // flash must be held for threshold samples to reset, and is cancled if released before that
    if (flash_held > STATUS_LEDS_FLASH_RESET_THRESHOLD) {
      user_event(USER_EVENT_RESET_CONFIRMED);
      user_reset();
    } else if (flash_held == 1) {
      // only set once, to keep flash pattern consistent
      user_event(USER_EVENT_RESET_REQUESTED);
    } else if (flash_released > 0) {
      user_event(USER_EVENT_RESET_CANCELED);
    }

    flash_released = 0;
  }
}

static int init_status_leds_task()
{
  if (xTaskCreate(&status_leds_main, "status-leds", STATUS_LEDS_TASK_STACK, NULL, STATUS_LEDS_TASK_PRIORITY, &status_leds_task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  }

  return 0;
}

int init_status_leds()
{
  int err;

#ifdef USER_LED
  if ((err = init_user_led())) {
    LOG_ERROR("init_user_led");
    return err;
  }
#endif

#ifdef FLASH_LED
  if ((err = init_flash_led())) {
    LOG_ERROR("init_flash_led");
    return err;
  }
#endif

#ifdef ALERT_LED
  if ((err = init_alert_led())) {
    LOG_ERROR("init_flash_led");
    return err;
  }
#endif

  if ((err = init_status_leds_task())) {
    LOG_ERROR("init_status_leds_task");
    return err;
  }

  return 0;
}

static void set_user_led(enum status_led_mode mode)
{
  if (!user_led) {
    return;
  } else if (status_led_mode(user_led, mode)) {
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

static void read_flash_led()
{
  int ret;

  if (!flash_led) {
    return;
  } else if ((ret = status_led_read(flash_led)) < 0) {
    LOG_WARN("status_led_mode");
  } else if (ret) {
    LOG_INFO("active");
  } else {
    LOG_INFO("inactive");
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

void status_led_event(enum user_event event)
{
  if (event < USER_EVENT_MAX) {
    set_user_led(user_event_led_mode[event]);
  }

  if (event & USER_EVENT_FLASH) {
    set_flash_led(STATUS_LED_FLASH);
  }

  if (event & USER_EVENT_ALERT) {
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
