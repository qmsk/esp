#include <spi_leds.h>
#include "spi_leds.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TEST_FRAME_RATE (25)
#define TEST_FRAME_TICKS (1000 / TEST_FRAME_RATE / portTICK_RATE_MS)

#define TEST_MODE_COLOR_FRAMES 25
#define TEST_MODE_CHASE_FRAMES 2

int spi_leds_test_chase_frame(struct spi_leds *spi_leds, unsigned frame, struct spi_led_color color)
{
  int err;

  switch (spi_leds_color_parameter_for_protocol(spi_leds->options.protocol)) {
    case SPI_LEDS_COLOR_NONE:
      break;

    case SPI_LEDS_COLOR_BRIGHTNESS:
      color.brightness = 255;
      break;

    case SPI_LEDS_COLOR_WHITE:
      color.white = 0;
      break;
  }

  // black
  if ((err = spi_leds_set_all(spi_leds, (struct spi_led_color){ }))) {
    return err;
  }

  if (frame >= spi_leds->options.count) {
    return 0; // end
  }

  if ((err = spi_leds_set(spi_leds, frame, color))) {
    return err;
  }

  return TEST_FRAME_TICKS * TEST_MODE_CHASE_FRAMES;
}

int spi_leds_test_color_frame(struct spi_leds *spi_leds, unsigned frame, struct spi_led_color color)
{
  int err;

  switch (spi_leds_color_parameter_for_protocol(spi_leds->options.protocol)) {
    case SPI_LEDS_COLOR_NONE:
      break;

    case SPI_LEDS_COLOR_BRIGHTNESS:
      color.brightness = 255;
      break;

    case SPI_LEDS_COLOR_WHITE:
      color.white = 0;
      break;
  }

  if ((err = spi_leds_set_all(spi_leds, color))) {
    return err;
  }

  if (frame < TEST_MODE_COLOR_FRAMES) {
    return TEST_FRAME_TICKS;
  } else {
    return 0;
  }
}

int spi_leds_test_black_frame(struct spi_leds *spi_leds, unsigned frame)
{
  int err;

  // black
  if ((err = spi_leds_set_all(spi_leds, (struct spi_led_color){ }))) {
    return err;
  }

  return 0;
}

/*
 * Return number of ticks for this frame, 0 for last frame, <0 on error.
 */
int spi_leds_set_test(struct spi_leds *spi_leds, enum spi_leds_test_mode mode, unsigned frame)
{
  switch (mode) {
    case TEST_MODE_BLACK:
      return spi_leds_test_black_frame(spi_leds, frame);

    case TEST_MODE_CHASE:
      return spi_leds_test_chase_frame(spi_leds, frame, (struct spi_led_color){
        .r = 255,
        .g = 255,
        .b = 255,
      });

    case TEST_MODE_BLACK_RED:
      return spi_leds_test_color_frame(spi_leds, frame, (struct spi_led_color){
        .r = (255 * frame / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_RED_YELLOW:
      return spi_leds_test_color_frame(spi_leds, frame, (struct spi_led_color){
        .r = 255,
        .g = (255 * frame / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_YELLOW_GREEN:
      return spi_leds_test_color_frame(spi_leds, frame, (struct spi_led_color){
        .r = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
        .g = 255,
      });

    case TEST_MODE_GREEN_CYAN:
      return spi_leds_test_color_frame(spi_leds, frame, (struct spi_led_color){
        .g = 255,
        .b = (255 * frame / TEST_MODE_COLOR_FRAMES)
      });

    case TEST_MODE_CYAN_BLUE:
      return spi_leds_test_color_frame(spi_leds, frame, (struct spi_led_color){
        .g = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
        .b = 255,
      });

    case TEST_MODE_BLUE_MAGENTA:
      return spi_leds_test_color_frame(spi_leds, frame, (struct spi_led_color){
        .b = 255,
        .r = (255 * frame / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_MAGENTA_RED:
      return spi_leds_test_color_frame(spi_leds, frame, (struct spi_led_color){
        .r = 255,
        .b = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_RED_BLACK:
      return spi_leds_test_color_frame(spi_leds, frame, (struct spi_led_color){
        .r = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
      });

    default:
      LOG_ERROR("unknown mode=%d", mode);
      return -1;
  }
}

int spi_leds_test(struct spi_leds *spi_leds, enum spi_leds_test_mode mode)
{
  TickType_t tick = xTaskGetTickCount();
  int ret, err;

  LOG_INFO("mode=%d @ tick=%u", mode, tick);

  for (unsigned frame = 0; ; frame++) {
    if ((ret = spi_leds_set_test(spi_leds, mode, frame)) < 0) {
      LOG_ERROR("spi_leds_set_test(%d, %u)", mode, frame);
      return ret;
    }

    if ((err = spi_leds_tx(spi_leds))) {
      LOG_ERROR("spi_leds_tx");
      return err;
    }

    if (ret == 0) {
      break;
    }

    vTaskDelayUntil(&tick, ret);
  }

  return 0;
}
