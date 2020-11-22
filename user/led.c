#include "led.h"
#include "led_config.h"
#include "led_cmd.h"

#include <drivers/gpio.h>
#include <lib/cmd.h>
#include <lib/logging.h>

#include <c_types.h>
#include <espressif/esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static struct led {
  os_timer_t timer;
  xTaskHandle task;
  xQueueHandle queue;

  enum led_mode mode;
  unsigned counter;

} led;

static inline void led_off()
{
  GPIO_Output(LED_GPIO, LED_INVERTED ? 1 : 0);
}

static inline void led_on()
{
  GPIO_Output(LED_GPIO, LED_INVERTED ? 0 : 1);
}

void led_timer(void *arg)
{
  struct led *led = arg;

  switch(led->mode) {
    case LED_OFF:
      led_off();
      break;

    case LED_SLOW:
      led->counter++;

      if (led->counter >= LED_BLINK_SLOW_CYCLE) {
        led->counter = 0;
        led_on();
      } else {
        led_off();
      }
      break;

    case LED_FAST:
      if (led->counter) {
        led->counter = 0;
        led_off();
      } else {
        led->counter = 1;
        led_on();
      }
      break;

    case LED_ON:
      led_on();
      break;

    case LED_BLINK:
      led_off();
      break;

  }
}

static inline void led_timer_repeat()
{
  os_timer_disarm(&led.timer);
  os_timer_arm(&led.timer, LED_TIMER_PERIOD, true);
}

static inline void led_timer_oneshot()
{
  os_timer_disarm(&led.timer);
  os_timer_arm(&led.timer, LED_TIMER_PERIOD, false);
}

static inline void led_timer_stop()
{
  os_timer_disarm(&led.timer);
}

static void led_update(enum led_mode mode) {
  led.mode = mode;

  switch (mode) {
    case LED_OFF:
      led_timer_stop();
      led_off();
      break;

    case LED_SLOW:
    case LED_FAST:
      led_off();
      led_timer_repeat();
      break;

    case LED_ON:
      led_timer_stop();
      led_on();
      break;

    case LED_BLINK:
      led_on();
      led_timer_oneshot();
      break;

    default:
      break;
  }
}

#define LED_TASK_STACK 128

void led_task(void *arg)
{
  struct led *led = arg;
  enum led_mode mode;

  for (;;) {
    if (!xQueueReceive(led->queue, &mode, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
      continue;
    }

    led_update(mode);
  }
}

int init_led(enum led_mode mode)
{
  GPIO_SetupOutput(LED_GPIO, GPIO_OUTPUT);

  // setup os_timer
  os_timer_setfn(&led.timer, led_timer, &led);

  // initial state
  led_update(mode);

  // setup task
  if ((led.queue = xQueueCreate(1, sizeof(enum led_mode))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if (xTaskCreate(&led_task, (signed char *) "led", LED_TASK_STACK, &led, tskIDLE_PRIORITY + 2, &led.task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  }

  return 0;
}

int led_set(enum led_mode mode)
{
  if (xQueueOverwrite(led.queue, &mode) <= 0) {
    LOG_ERROR("xQueueOverwrite");
  }

  return 0;
}

int led_cmd_off(int argc, char **argv, void *ctx)
{
  led_set(LED_OFF);

  return 0;
}

int led_cmd_slow(int argc, char **argv, void *ctx)
{
  led_set(LED_SLOW);

  return 0;
}

int led_cmd_fast(int argc, char **argv, void *ctx)
{
  led_set(LED_FAST);

  return 0;
}

int led_cmd_blink(int argc, char **argv, void *ctx)
{
  led_set(LED_BLINK);

  return 0;
}

int led_cmd_on(int argc, char **argv, void *ctx)
{
  led_set(LED_ON);

  return 0;
}

const struct cmd led_commands[] = {
  { "off",    led_cmd_off,      .describe = "Turn off LED" },
  { "slow",   led_cmd_slow,     .describe = "Blink LED slowly" },
  { "fast",   led_cmd_fast,     .describe = "Blink LED fast" },
  { "blink",  led_cmd_blink,    .describe = "Blink LED once" },
  { "on",     led_cmd_on,       .describe = "Turn on LED" },
  {}
};

const struct cmdtab led_cmdtab = {
  .commands = led_commands,
};
