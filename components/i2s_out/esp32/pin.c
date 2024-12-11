#include "../i2s_out.h"

#include <esp_rom_gpio.h>
#include <hal/gpio_ll.h>
#include <hal/rtc_io_ll.h>
#include <soc/gpio_periph.h>
#include <soc/rtc_io_periph.h>

#include <logging.h>

static const uint8_t i2s_bck_out_sig[I2S_PORT_MAX] = {
  [I2S_PORT_0]  = I2S0O_BCK_OUT_IDX,
  [I2S_PORT_1]  = I2S1O_BCK_OUT_IDX,
};

static const uint8_t i2s_serial_data_out_sig[I2S_PORT_MAX] = {
  [I2S_PORT_0]  = I2S0O_DATA_OUT23_IDX,
  [I2S_PORT_1]  = I2S1O_DATA_OUT23_IDX,
};

static const uint8_t i2s_parallel8_data_out_sig[I2S_PORT_MAX] = {
  [I2S_PORT_1]  = I2S1O_DATA_OUT0_IDX, // special 8-bit mode on I2S1 only
};

static void setup_rtc_pin(gpio_num_t gpio)
{
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
  int rtc_ionum;

  if ((rtc_ionum = rtc_io_num_map[gpio]) < 0) {
    return;
  }

  rtcio_ll_pullup_disable(rtc_ionum);
  rtcio_ll_pulldown_enable(rtc_ionum);

  // configure RTC IOMUX as digital GPIO
  rtcio_ll_function_select(rtc_ionum, RTCIO_FUNC_DIGITAL);
#endif
}

static void setup_gpio_pin(gpio_num_t gpio)
{
  gpio_ll_iomux_func_sel(GPIO_PIN_MUX_REG[gpio], PIN_FUNC_GPIO);
  gpio_ll_input_disable(&GPIO, gpio);
  gpio_ll_output_enable(&GPIO, gpio);
  gpio_ll_pullup_dis(&GPIO, gpio);
  gpio_ll_pulldown_en(&GPIO, gpio);
}

static void clear_gpio_pin(gpio_num_t gpio)
{
  gpio_ll_set_level(&GPIO, gpio, 0);
}

int i2s_out_pin_init(struct i2s_out *i2s_out)
{

  for (int i = 0; i < I2S_OUT_PARALLEL_SIZE; i++) {
    i2s_out->bck_gpios[i] = -1;
    i2s_out->data_gpios[i] = -1;
    i2s_out->inv_data_gpios[i] = -1;
  }

  return 0;
}

int i2s_out_pin_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options)
{
  if (options->pin_mutex) {
    if (!xSemaphoreTake(options->pin_mutex, options->pin_timeout)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      i2s_out->pin_mutex = options->pin_mutex;
    }
  }

  switch (options->mode) {
    case I2S_OUT_MODE_16BIT_SERIAL:
      LOG_DEBUG("port=%d: mode=I2S_OUT_MODE_16BIT_SERIAL bck_gpio=%d serial_data_gpio=%d inv_data_gpio=%d", i2s_out->port,
        options->bck_gpio,
        options->data_gpio,
        options->inv_data_gpio
      );

      break;

    case I2S_OUT_MODE_32BIT_SERIAL:
      LOG_DEBUG("port=%d: mode=I2S_OUT_MODE_32BIT_SERIAL bck_gpio=%d serial_data_gpio=%d inv_data_gpio=%d", i2s_out->port,
        options->bck_gpio,
        options->data_gpio,
        options->inv_data_gpio
      );

      break;

    case I2S_OUT_MODE_8BIT_PARALLEL:
      if (!i2s_parallel8_data_out_sig[i2s_out->port]) {
        LOG_ERROR("unsupported mode=I2S_OUT_MODE_8BIT_PARALLEL for port=%d", i2s_out->port);
        return -1;
      }

      LOG_DEBUG("port=%d: mode=I2S_OUT_MODE_8BIT_PARALLEL bck_gpio=%d parallel_data_gpio=[%d, %d, %d, %d, %d, %d, %d, %d]", i2s_out->port,
        options->bck_gpio,
        options->data_gpios[0],
        options->data_gpios[1],
        options->data_gpios[2],
        options->data_gpios[3],
        options->data_gpios[4],
        options->data_gpios[5],
        options->data_gpios[6],
        options->data_gpios[7]
      );

      break;

    default:
      LOG_ERROR("invalid mode=%d", options->mode);
      return -1;
  }

  taskENTER_CRITICAL(&i2s_out->mux);

  switch (options->mode) {
    case I2S_OUT_MODE_16BIT_SERIAL:
    case I2S_OUT_MODE_32BIT_SERIAL:
      if (options->bck_gpio > 0) {
        i2s_out->bck_gpio_inv = options->bck_inv;
        i2s_out->bck_gpios[0] = options->bck_gpio;

        setup_gpio_pin(options->bck_gpio);
        setup_rtc_pin(options->bck_gpio);

        esp_rom_gpio_connect_out_signal(options->bck_gpio, i2s_bck_out_sig[i2s_out->port], options->bck_inv, false);
      }

      if (options->data_gpio > 0) {
        i2s_out->data_gpios[0] = options->data_gpio;

        setup_gpio_pin(options->data_gpio);
        setup_rtc_pin(options->data_gpio);

        esp_rom_gpio_connect_out_signal(options->data_gpio, i2s_serial_data_out_sig[i2s_out->port], false, false);
      }

      if (options->inv_data_gpio > 0) {
        i2s_out->inv_data_gpios[0] = options->inv_data_gpio;

        setup_gpio_pin(options->inv_data_gpio);
        setup_rtc_pin(options->inv_data_gpio);

        // invert gpio output pin
        esp_rom_gpio_connect_out_signal(options->inv_data_gpio, i2s_serial_data_out_sig[i2s_out->port], true, false);
      }

      break;

    case I2S_OUT_MODE_8BIT_PARALLEL:
      for (int i = 0; i < I2S_OUT_PARALLEL_SIZE; i++) {
        if (options->bck_gpios[i] > 0) {
          i2s_out->bck_gpio_inv = options->bck_inv;
          i2s_out->bck_gpios[i] = options->bck_gpios[i];

          setup_gpio_pin(options->bck_gpios[i]);
          setup_rtc_pin(options->bck_gpios[i]);

          esp_rom_gpio_connect_out_signal(options->bck_gpios[i], i2s_bck_out_sig[i2s_out->port], options->bck_inv, false);
        }

        if (options->data_gpios[i] > 0) {
          i2s_out->data_gpios[i] = options->data_gpios[i];

          setup_gpio_pin(options->data_gpios[i]);
          setup_rtc_pin(options->data_gpios[i]);

          // data[0] is mapped to the most significant bit, which is OUT7
          esp_rom_gpio_connect_out_signal(options->data_gpios[i], i2s_parallel8_data_out_sig[i2s_out->port] + 8 - i - 1, false, false);
        }

        if (options->inv_data_gpios[i] > 0) {
          i2s_out->inv_data_gpios[i] = options->inv_data_gpios[i];

          setup_gpio_pin(options->inv_data_gpios[i]);
          setup_rtc_pin(options->inv_data_gpios[i]);

          // data[0] is mapped to the most significant bit, which is OUT7
          esp_rom_gpio_connect_out_signal(options->inv_data_gpios[i], i2s_parallel8_data_out_sig[i2s_out->port] + 8 - i - 1, true, false);
        }
      }
  }

  taskEXIT_CRITICAL(&i2s_out->mux);

  return 0;
}

void i2s_out_pin_teardown(struct i2s_out *i2s_out)
{
  LOG_DEBUG("");

  taskENTER_CRITICAL(&i2s_out->mux);

  // place output into a safe state

  for (int i = 0; i < I2S_OUT_PARALLEL_SIZE; i++) {
    if (i2s_out->bck_gpios[i] >= 0) {
      clear_gpio_pin(i2s_out->bck_gpios[i]);

      esp_rom_gpio_connect_out_signal(i2s_out->bck_gpios[i], SIG_GPIO_OUT_IDX, i2s_out->bck_gpio_inv, false);

      i2s_out->bck_gpios[i] = -1;
    }

    if (i2s_out->data_gpios[i] >= 0) {
      clear_gpio_pin(i2s_out->data_gpios[i]);

      esp_rom_gpio_connect_out_signal(i2s_out->data_gpios[i], SIG_GPIO_OUT_IDX, false, false);

      i2s_out->data_gpios[i] = -1;
    }

    if (i2s_out->inv_data_gpios[i] >= 0) {
      clear_gpio_pin(i2s_out->inv_data_gpios[i]);

      esp_rom_gpio_connect_out_signal(i2s_out->inv_data_gpios[i], SIG_GPIO_OUT_IDX, true, false);

      i2s_out->inv_data_gpios[i] = -1;
    }
  }

  taskEXIT_CRITICAL(&i2s_out->mux);

  if (i2s_out->pin_mutex) {
    xSemaphoreGive(i2s_out->pin_mutex);

    i2s_out->pin_mutex = NULL;
  }
}
