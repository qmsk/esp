#include "led.h"
#include "led_config.h"
#include "led_cmd.h"

#include <drivers/gpio.h>
#include <lib/cmd.h>

#include <c_types.h>
#include <espressif/esp_timer.h>

static struct led {
  os_timer_t timer;

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

int init_led(enum led_mode mode)
{
  GPIO_SetupOutput(LED_GPIO, GPIO_OUTPUT);

  // setup os_timer
  os_timer_setfn(&led.timer, led_timer, &led);

  return led_set(mode);
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

int led_set(enum led_mode mode)
{
  led.mode = mode;

  switch (mode) {
    case LED_OFF:
      led_timer_stop();
      led_off();
      return 0;

    case LED_SLOW:
    case LED_FAST:
      led_off();
      led_timer_repeat();
      return 0;

    case LED_ON:
      led_timer_stop();
      led_on();
      return 0;

    case LED_BLINK:
      led_on();
      led_timer_oneshot();

    default:
      return -1;
  }
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
