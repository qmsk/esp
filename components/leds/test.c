#include <leds.h>
#include "leds.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TEST_FRAME_RATE (25)
#define TEST_FRAME_TICKS (1000 / TEST_FRAME_RATE / portTICK_RATE_MS)

#define TEST_MODE_COLOR_FRAMES 25
#define TEST_MODE_CHASE_FRAMES 2

int leds_test_chase_frame(struct leds *leds, unsigned frame, struct leds_color color)
{
  int err;

  switch (leds_color_parameter_for_protocol(leds->options.protocol)) {
    case LEDS_COLOR_NONE:
      break;

    case LEDS_COLOR_DIMMER:
      color.dimmer = 255;
      break;

    case LEDS_COLOR_WHITE:
      color.white = 0;
      break;
  }

  // black
  if ((err = leds_set_all(leds, (struct leds_color){ }))) {
    return err;
  }

  if (frame >= leds->options.count) {
    return 0; // end
  }

  if ((err = leds_set(leds, frame, color))) {
    return err;
  }

  return TEST_FRAME_TICKS * TEST_MODE_CHASE_FRAMES;
}

int leds_test_color_frame(struct leds *leds, unsigned frame, struct leds_color color)
{
  int err;

  switch (leds_color_parameter_for_protocol(leds->options.protocol)) {
    case LEDS_COLOR_NONE:
      break;

    case LEDS_COLOR_DIMMER:
      color.dimmer = 255;
      break;

    case LEDS_COLOR_WHITE:
      color.white = 0;
      break;
  }

  if ((err = leds_set_all(leds, color))) {
    return err;
  }

  if (frame < TEST_MODE_COLOR_FRAMES) {
    return TEST_FRAME_TICKS;
  } else {
    return 0;
  }
}

int leds_test_black_frame(struct leds *leds, unsigned frame)
{
  int err;

  // black
  if ((err = leds_set_all(leds, (struct leds_color){ }))) {
    return err;
  }

  return 0;
}

/*
 * Return number of ticks for this frame, 0 for last frame, <0 on error.
 */
int leds_set_test(struct leds *leds, enum leds_test_mode mode, unsigned frame)
{
  switch (mode) {
    case TEST_MODE_BLACK:
      return leds_test_black_frame(leds, frame);

    case TEST_MODE_CHASE:
      return leds_test_chase_frame(leds, frame, (struct leds_color){
        .r = 255,
        .g = 255,
        .b = 255,
      });

    case TEST_MODE_BLACK_RED:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .r = (255 * frame / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_RED_YELLOW:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .r = 255,
        .g = (255 * frame / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_YELLOW_GREEN:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .r = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
        .g = 255,
      });

    case TEST_MODE_GREEN_CYAN:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .g = 255,
        .b = (255 * frame / TEST_MODE_COLOR_FRAMES)
      });

    case TEST_MODE_CYAN_BLUE:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .g = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
        .b = 255,
      });

    case TEST_MODE_BLUE_MAGENTA:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .b = 255,
        .r = (255 * frame / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_MAGENTA_RED:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .r = 255,
        .b = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_RED_BLACK:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .r = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_END:
      return leds_test_black_frame(leds, frame);

    default:
      LOG_ERROR("unknown mode=%d", mode);
      return -1;
  }
}
