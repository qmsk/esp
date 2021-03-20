#include "spi_leds.h"

#include <logging.h>
#include <spi_leds.h>

extern struct spi_leds *spi_leds;

int spi_leds_cmd_clear(int argc, char **argv, void *ctx)
{
  int err;

  if (!spi_leds) {
    LOG_WARN("disabled");
    return 1;
  }

  struct spi_led_color spi_led_color = { }; // off

  if ((err = spi_leds_set_all(spi_leds, spi_led_color))) {
    LOG_ERROR("spi_leds_set_all");
    return err;
  }

  if ((err = update_spi_leds())) {
    LOG_ERROR("update_spi_leds");
    return err;
  }

  return 0;
}

int spi_leds_cmd_all(int argc, char **argv, void *ctx)
{
  int rgb, a = 0xff;
  int err;

  if (!spi_leds) {
    LOG_WARN("disabled");
    return 1;
  }

  if ((err = cmd_arg_int(argc, argv, 1, &rgb)))
    return err;
  if ((argc > 2) && (err = cmd_arg_int(argc, argv, 2, &a)))
    return err;

  struct spi_led_color spi_led_color = {
    .r = (rgb >> 16) & 0xFF,
    .g = (rgb >>  8) & 0xFF,
    .b = (rgb >>  0) & 0xFF,

    .parameters.brightness = a,
  };

  if ((err = spi_leds_set_all(spi_leds, spi_led_color))) {
    LOG_ERROR("spi_leds_set_all");
    return err;
  }

  if ((err = update_spi_leds())) {
    LOG_ERROR("update_spi_leds");
    return err;
  }

  return 0;
}

int spi_leds_cmd_set(int argc, char **argv, void *ctx)
{
  unsigned index;
  int rgb, a = 0xff;
  int err;

  if (!spi_leds) {
    LOG_WARN("disabled");
    return 1;
  }

  if ((err = cmd_arg_uint(argc, argv, 1, &index)))
    return err;
  if ((err = cmd_arg_int(argc, argv, 2, &rgb)))
    return err;
  if ((argc > 3) && (err = cmd_arg_int(argc, argv, 3, &a)))
    return err;

  struct spi_led_color spi_led_color = {
    .r = (rgb >> 16) & 0xFF,
    .g = (rgb >>  8) & 0xFF,
    .b = (rgb >>  0) & 0xFF,

    .parameters.brightness = a,
  };

  if ((err = spi_leds_set(spi_leds, index, spi_led_color))) {
    LOG_ERROR("spi_leds_set");
    return err;
  }

  if ((err = update_spi_leds())) {
    LOG_ERROR("update_spi_leds");
    return err;
  }

  return 0;
}

const struct cmd spi_leds_commands[] = {
  { "clear",  spi_leds_cmd_clear, .usage = "",               .describe = "Clear values" },
  { "all",    spi_leds_cmd_all,   .usage = "RGB [A]",        .describe = "Set all pixels to values" },
  { "set",    spi_leds_cmd_set,   .usage = "INDEX RGB [A]",  .describe = "Set one pixel to value" },
  { }
};

const struct cmdtab spi_leds_cmdtab = {
  .commands = spi_leds_commands,
};
