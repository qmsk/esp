#include <leds.h>
#include "leds.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <math.h>

#define TEST_FRAME_RATE (25)
#define TEST_FRAME_TICKS (1000 / TEST_FRAME_RATE / portTICK_RATE_MS)

#define TEST_MODE_COLOR_FRAMES 25
#define TEST_MODE_CHASE_FRAMES 2
#define TEST_MODE_RAINBOW_FADEIN_FRAMES 25

int leds_test_chase_frame(struct leds *leds, unsigned frame, struct leds_color color)
{
  int err;

  switch (leds_parameter_type(leds)) {
    case LEDS_PARAMETER_NONE:
      break;

    case LEDS_PARAMETER_DIMMER:
      color.dimmer = 255;
      break;

    case LEDS_PARAMETER_WHITE:
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

  switch (leds_parameter_type(leds)) {
    case LEDS_PARAMETER_NONE:
      break;

    case LEDS_PARAMETER_DIMMER:
      color.dimmer = 255;
      break;

    case LEDS_PARAMETER_WHITE:
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

/* Convert a value i from 0..c into a value between 0.0 .. 1.0 with a phase offset of a/b+c */
static inline float interval(unsigned i, unsigned n)
{
  return (float)(i) / (float)(n);
}

/* Convert a value i from 0..c into a value between 0.0 .. 1.0 with a phase offset of a/b+c */
static inline float phased_interval(unsigned i, unsigned n, unsigned a, unsigned b, unsigned c)
{
  return (float)((i + a * n / b + c) % n) / (float)(n);
}

/* Convert a value x 0.0 .. 1.0 into a triangle wave between -1.0 ... +1.0 */
static inline float interval2wave(float x)
{
  return fabsf(x * 2.0 - 1.0) * 3.0 - 1.0;
}

/* Clamp any value to between 0.0 .. 1.0 */
static inline float clamp(float x)
{
  if (x < 0.0) {
    return 0.0;
  } else if (x > 1.0) {
    return 1.0;
  } else {
    return x;
  }
}

static inline uint8_t f2u8(float x) {
  return x * 255;
}

int leds_test_rainbow_frame(struct leds *leds, unsigned frame)
{
  unsigned count = leds->options.count;
  struct leds_color color;

  switch (leds_parameter_type(leds)) {
    case LEDS_PARAMETER_NONE:
      break;

    case LEDS_PARAMETER_DIMMER:
      color.dimmer = 255;
      break;

    case LEDS_PARAMETER_WHITE:
      color.white = 0;
      break;
  }

  for (unsigned i = 0; i < count; i++) {
    float r1 = phased_interval(i, count, 0, 3, frame);
    float g1 = phased_interval(i, count, 1, 3, frame);
    float b1 = phased_interval(i, count, 2, 3, frame);

    float r2 = interval2wave(r1);
    float g2 = interval2wave(g1);
    float b2 = interval2wave(b1);

    float r3 = clamp(r2);
    float g3 = clamp(g2);
    float b3 = clamp(b2);

    if (frame < TEST_MODE_RAINBOW_FADEIN_FRAMES) {
      r3 *= interval(frame, TEST_MODE_RAINBOW_FADEIN_FRAMES);
      g3 *= interval(frame, TEST_MODE_RAINBOW_FADEIN_FRAMES);
      b3 *= interval(frame, TEST_MODE_RAINBOW_FADEIN_FRAMES);
    }

    color.r = r3 * 255;
    color.g = g3 * 255;
    color.b = b3 * 255;

    leds_set(leds, i, color);
  }

  return TEST_FRAME_TICKS;
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
    case TEST_MODE_NONE:
      return 0;

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

    case TEST_MODE_BLACK_WHITE:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .white = (255 * frame / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_WHITE_RGBW:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .r     = (255 * frame / TEST_MODE_COLOR_FRAMES),
        .g     = (255 * frame / TEST_MODE_COLOR_FRAMES),
        .b     = (255 * frame / TEST_MODE_COLOR_FRAMES),
        .white = 255,
      });

    case TEST_MODE_RGBW_RGB:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .r     = 255,
        .g     = 255,
        .b     = 255,
        .white = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
      });

    case TEST_MODE_RGB_BLACK:
      return leds_test_color_frame(leds, frame, (struct leds_color){
        .r     = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
        .g     = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
        .b     = (255 * (TEST_MODE_COLOR_FRAMES - frame) / TEST_MODE_COLOR_FRAMES),
        .white = 0,
      });

    case TEST_MODE_RAINBOW:
      return leds_test_rainbow_frame(leds, frame);

    case TEST_MODE_BLACK:
      return leds_test_black_frame(leds, frame);

    default:
      LOG_ERROR("unknown mode=%d", mode);
      return -1;
  }
}
