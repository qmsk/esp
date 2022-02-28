#pragma once

#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP8266
# include <driver/gpio.h>
#elif CONFIG_IDF_TARGET_ESP32
# include <hal/gpio_types.h>
#endif
#include <freertos/FreeRTOS.h>

enum atx_psu_bit {
  ATX_PSU_BIT_0,
  ATX_PSU_BIT_1,
  ATX_PSU_BIT_2,
  ATX_PSU_BIT_3,
  ATX_PSU_BIT_4,
  ATX_PSU_BIT_5,
  ATX_PSU_BIT_6,
  ATX_PSU_BIT_7,

  ATX_PSU_BIT_COUNT
};

struct atx_psu;

struct atx_psu_options {
  /*
   * Assumes an active-high NPN transistor:
   *  ATX POWER_EN -> collector
   *  gpio -> 1k ohm -> base
   *  GND <- emitter
   *
   * Use <0 to disable.
   */
  gpio_num_t power_enable_gpio;

  /*
   * Assumes an active-low NPN transistor:
   *  gpio -> collector -> 10k -> 3V3
   *  ATX POWER_GOOD -> 1k -> base
   *  emitter -> GND
   *
   * Use <0 to disable.
   */
  gpio_num_t power_good_gpio;

  /*
   * Return ATX PSU into standby mode once all bits are inactive, and the timeout has passed.
   */
  TickType_t timeout;
};

int atx_psu_new(struct atx_psu **atx_psup, struct atx_psu_options options);

/* Run as a separate task */
void atx_psu_main(void *arg);

/*
 * Enable ATX power immediately, as soon as one bit is activated.
 */
void atx_psu_power_enable(struct atx_psu *atx_psu, enum atx_psu_bit bit);

/*
 * Enable ATX power immediately, as soon as one bit is activated.
 *
 * Wait for ATX power good signal, if power_good_gpio is configured.
 *
 * @return 0 on timeout, >0 when power good.
 */
int atx_psu_power_good(struct atx_psu *atx_psu, enum atx_psu_bit bit, TickType_t timeout);

/*
 * Disable ATX power and return to standby mode, once all bits are inactive and the timeout has passed.
 */
void atx_psu_standby(struct atx_psu *atx_psu, enum atx_psu_bit bit);
