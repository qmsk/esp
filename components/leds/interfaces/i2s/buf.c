#include "../i2s.h"

#include <logging.h>

size_t leds_interface_i2s_buffer_size(enum leds_interface_i2s_mode mode, unsigned led_count, unsigned parallel)
{
  unsigned count;

#if I2S_OUT_PARALLEL_SUPPORTED
  if (parallel) {
    // assuming 8-bit parallel output
    count = led_count / parallel;
    parallel = 8;
  } else {
    // serial output
    count = led_count;
    parallel = 1;
  }
#else
  // serial output
  count = led_count;
  parallel = 1;
#endif

  switch (mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
      // including start frame, excluding end frame (generated using EOF DMA)
      return parallel * (1 + count) * sizeof(uint32_t);

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
      return parallel * count * sizeof(uint16_t[6]);

    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
      return parallel * count * sizeof(uint16_t[8]);

    default:
      LOG_FATAL("invalid mode=%d", mode);
  }
}

size_t leds_interface_i2s_buffer_align(enum leds_interface_i2s_mode mode, unsigned parallel)
{
  switch(mode) {
    case LEDS_INTERFACE_I2S_MODE_32BIT_BCK:
    #if I2S_OUT_PARALLEL_SUPPORTED
      if (parallel) {
        return I2S_OUT_WRITE_PARALLEL8X32_ALIGN;
      } else {
        return I2S_OUT_WRITE_SERIAL32_ALIGN;
      }
    #else
      return I2S_OUT_WRITE_SERIAL16_ALIGN;
    #endif

    case LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL:
    case LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL:
    #if I2S_OUT_PARALLEL_SUPPORTED
      if (parallel) {
        return I2S_OUT_WRITE_PARALLEL8X16_ALIGN;
      } else {
        return I2S_OUT_WRITE_SERIAL16_ALIGN;
      }
    #else
      return I2S_OUT_WRITE_SERIAL16_ALIGN;
    #endif

    default:
      LOG_FATAL("invalid mode=%d", mode);
  }
}
