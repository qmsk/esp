#include "led.h"
#include "led_config.h"
#include "led_cmd.h"

#include <drivers/gpio.h>
#include <lib/cmd.h>
#include <lib/logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static struct led {
  xTaskHandle task;
  xQueueHandle queue;
  portTickType tick;

  enum led_mode mode;
  unsigned state;

} led;

static inline void led_off()
{
  GPIO_Output(LED_GPIO, LED_INVERTED ? 1 : 0);
}

static inline void led_on()
{
  GPIO_Output(LED_GPIO, LED_INVERTED ? 0 : 1);
}

static void led_switch(struct led *led, enum led_mode mode)
{
  switch(led->mode = mode) {
    case LED_OFF:
      led_off();
      break;

    case LED_ON:
      led_on();
      break;

    case LED_SLOW:
    case LED_FAST:
    case LED_BLINK:
      led->state = 0;
      led_on();
      break;
  }
}

static portTickType led_tick(struct led *led)
{
  switch(led->mode) {
    case LED_OFF:
    case LED_ON:
      return 0;

    case LED_SLOW:
      if (led->state) {
        led->state = 0;
        led_off();
        return LED_BLINK_SLOW_PERIOD_OFF / portTICK_RATE_MS;
      } else {
        led->state = 1;
        led_on();
        return LED_BLINK_SLOW_PERIOD_ON / portTICK_RATE_MS;
      }

    case LED_FAST:
      if (led->state) {
        led->state = 0;
        led_off();
      } else {
        led->state = 1;
        led_on();
      }
      return LED_BLINK_FAST_PERIOD / portTICK_RATE_MS;

    case LED_BLINK:
      if (led->state) {
        led_off();
        return 0;
      } else {
        led->state = 1;
        return LED_BLINK_PERIOD / portTICK_RATE_MS;
      }

    default:
      return 0;
  }
}

// schedule next tick
static portTickType led_schedule(struct led *led, portTickType period)
{
  portTickType tick = xTaskGetTickCount();

  if (period == 0) {
    // reset phase
    led->tick = 0;

    // indefinite period
    return portMAX_DELAY;

  } else if (led->tick == 0) {
    // start phase
    led->tick = tick + period;

    return period;

  } else if (led->tick + period > tick) {
    led->tick += period;

    return led->tick - tick;

  } else {
    led->tick += period;

    // missed tick, catchup
    return 0;
  }
}

#define LED_TASK_STACK 128

void led_task(void *arg)
{
  struct led *led = arg;
  enum led_mode mode;

  led->tick = 0;

  for (;;) {
    portTickType period = led_tick(led);
    portTickType delay = led_schedule(led, period);

    if (xQueueReceive(led->queue, &mode, delay)) {
      LOG_DEBUG("xQueueReceive -> update")
      led_switch(led, mode);
    } else {
      LOG_DEBUG("xQueueReceive -> tick");
    }
  }
}

int init_led(enum led_mode mode)
{
  GPIO_SetupOutput(LED_GPIO, GPIO_OUTPUT);

  // set initial state
  led_switch(&led, mode);

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

/* CLI */
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
