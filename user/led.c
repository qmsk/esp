#include "led.h"
#include "led_config.h"
#include "led_cmd.h"

#include <drivers/gpio.h>

#include <lib/cmd.h>

static inline void led_off()
{
  GPIO_Output(LED_GPIO, LED_INVERTED ? 1 : 0);
}

static inline void led_on()
{
  GPIO_Output(LED_GPIO, LED_INVERTED ? 0 : 1);
}

int init_led(enum led_state state)
{
  GPIO_SetupOutput(LED_GPIO, GPIO_OUTPUT);

  return led_set(state);
}

int led_set(enum led_state state)
{
  switch (state) {
    case LED_OFF:
      led_off();
      return 0;

    case LED_ON:
      led_on();
      return 0;

    default:
      return -1;
  }
}

int led_cmd_off(int argc, char **argv, void *ctx)
{
  led_off();

  return 0;
}

int led_cmd_on(int argc, char **argv, void *ctx)
{
  led_on();

  return 0;
}

const struct cmd led_commands[] = {
  { "off",    led_cmd_off,    .describe = "Turn off LED" },
  { "on",     led_cmd_on,     .describe = "Turn on LED" },
  {}
};

const struct cmdtab led_cmdtab = {
  .commands = led_commands,
};
