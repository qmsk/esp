#include "../i2s.h"
#include "../../leds.h"
#include "../../stats.h"
#include "../../gpio.h"

#include <logging.h>

int leds_interface_i2s_init(struct leds_interface_i2s *interface, const struct leds_interface_i2s_options *options, enum leds_interface_i2s_mode mode, union leds_interface_i2s_func func, unsigned count, struct leds_interface_i2s_stats *stats)
{
  interface->mode = mode;
  interface->func = func;

#if LEDS_I2S_PARALLEL_ENABLED
  interface->parallel = options->parallel;
#else
  interface->parallel = 0;
#endif
  interface->repeat = options->repeat;

  if (!(interface->buf = calloc(1, leds_interface_i2s_buf_size(interface->mode, interface->parallel)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  interface->i2s_out = options->i2s_out;
  interface->i2s_out_options = (struct i2s_out_options) {
    // shared IO pins
    .pin_mutex    = options->pin_mutex,
    .pin_timeout  = options->pin_timeout,
  };

  switch(mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
    #if LEDS_I2S_PARALLEL_ENABLED
      if (options->parallel) {
        interface->i2s_out_options.mode = I2S_OUT_MODE_8BIT_PARALLEL;
      } else {
        interface->i2s_out_options.mode = I2S_OUT_MODE_32BIT_SERIAL;
      }
    #else
      interface->i2s_out_options.mode = I2S_OUT_MODE_32BIT_SERIAL;
    #endif

      break;

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U200_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
    #if LEDS_I2S_PARALLEL_ENABLED
      if (options->parallel) {
        interface->i2s_out_options.mode = I2S_OUT_MODE_8BIT_PARALLEL;
      } else {
        // using 4x4bit -> 16-bit samples
        interface->i2s_out_options.mode = I2S_OUT_MODE_16BIT_SERIAL;
      }
    #else
      interface->i2s_out_options.mode = I2S_OUT_MODE_16BIT_SERIAL;
    #endif

      break;

    default:
      LOG_FATAL("unknown mode=%d", mode);
  }

  switch(mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
      interface->i2s_out_options.clock = i2s_out_clock(options->clock_rate);

      break;

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U200_4X4_80UL:
      // 3.333MHz bit clock => 0.300us per I2S bit
      // four I2S bits per 1.20us protocol bit
      interface->i2s_out_options.clock = I2S_OUT_CLOCK_3M333;

      break;

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
      // 3.2MHz bit clock => 0.3125us per I2S bit
      // four I2S bits per 1.25us protocol bit
      interface->i2s_out_options.clock = I2S_OUT_CLOCK_3M2;

      break;

    default:
      LOG_FATAL("unknown mode=%d", mode);
  }

  switch (mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
      // XXX: required to workaround I2S start glitch looping previous data bits
      interface->i2s_out_options.eof_value = 0x00000000;

      // one clock cycle per pixel, min 32 cycles
      interface->i2s_out_options.eof_count = (1 + count / 32);

      break;

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U200_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
      // 1.25us per 4-bit = 2.5us per byte * four bytes per I2S sample = 10us per 32-bit I2S sample
      interface->i2s_out_options.eof_value = 0x00000000;

      // hold low for 8 * 10us
      interface->i2s_out_options.eof_count = 8;

      break;

    default:
      LOG_FATAL("unknown mode=%d", interface->mode);
  }


#if LEDS_I2S_GPIO_PINS_ENABLED
  for (int i = 0; i < LEDS_I2S_GPIO_PINS_SIZE; i++) {
    interface->i2s_out_options.bck_gpios[i] = (i < options->gpio_pins_count) ? options->clock_pins[i] : GPIO_NUM_NC;
    interface->i2s_out_options.data_gpios[i] = (i < options->gpio_pins_count) ? options->data_pins[i] : GPIO_NUM_NC;
    interface->i2s_out_options.inv_data_gpios[i] = (i < options->gpio_pins_count) ? options->inv_data_pins[i] : GPIO_NUM_NC;
  }
#endif

#if LEDS_I2S_PARALLEL_ENABLED
  // allow multiple copies of parallel data bits on different GPIOs
  interface->i2s_out_options.parallel_data_bits = options->parallel;

  if (options->parallel) {
    // I2S LCD mode requires the BCK/WS signal to be inverted when routed through the GPIO matrix
    // invert BCK to idle high, transition on falling edge, and sample data on rising edge
    interface->i2s_out_options.bck_inv = true;
  } else {
    // BCK is idle low, transition on falling edge, and sample data on rising edge
    interface->i2s_out_options.bck_inv = false;
  }
#endif

  interface->gpio = options->gpio;
  interface->stats = stats;
  interface->timeout = options->timeout;

  return 0;
}

int leds_interface_i2s_setup(struct leds_interface_i2s *interface)
{
  int err = 0;

  WITH_STATS_TIMER(&interface->stats->open) {
    if ((err = i2s_out_open(interface->i2s_out, &interface->i2s_out_options, interface->timeout))) {
      LOG_ERROR("i2s_out_open");
      return err;
    }
  }

  interface->setup = true;

#if CONFIG_LEDS_GPIO_ENABLED
  leds_gpio_setup(&interface->gpio);
#endif

  return 0;
}

int leds_interface_i2s_close(struct leds_interface_i2s *interface)
{
  int err = 0;

  interface->setup = false;

#if CONFIG_LEDS_GPIO_ENABLED
  leds_gpio_close(&interface->gpio);
#endif

  if ((err = i2s_out_close(interface->i2s_out, interface->timeout))) {
    LOG_ERROR("i2s_out_close");
    return err;
  }

  return err;

}
