#pragma once

#include <gpio.h>

#if CONFIG_IDF_TARGET_ESP8266
  #include <driver/gpio.h>
#else
  #include <hal/gpio_types.h>
#endif

#if GPIO_I2C_ENABLED
  #include <freertos/FreeRTOS.h>
  #include <freertos/semphr.h>

  struct gpio_i2c_pca54xx_state {
    uint8_t output;
    uint8_t inversion;
    uint8_t config;
  };

  union gpio_i2c_state {
     struct gpio_i2c_pca54xx_state pca54xx;
  };

  struct gpio_i2c_dev {
    enum gpio_i2c_type type;
    struct gpio_i2c_options options;
    SemaphoreHandle_t mutex;
    union gpio_i2c_state state;
    const struct gpio_options *intr_pins[GPIO_I2C_PINS_MAX];
  };
#endif

/* gpio_host.c */
void gpio_host_intr_handler (const struct gpio_options *options, gpio_pins_t pins);

int gpio_host_setup_intr_pin(gpio_pin_t gpio, gpio_int_type_t int_type);
int gpio_host_setup(const struct gpio_options *options);
int gpio_host_setup_input(const struct gpio_options *options, gpio_pins_t pins);
int gpio_host_get(const struct gpio_options *options, gpio_pins_t *pins);
int gpio_host_setup_output(const struct gpio_options *options, gpio_pins_t pins);
int gpio_host_clear(const struct gpio_options *options);
int gpio_host_set(const struct gpio_options *options, gpio_pins_t pins);
int gpio_host_set_all(const struct gpio_options *options);

/* gpio_intr.c */
int gpio_intr_init();
void gpio_intr_setup_pin(const struct gpio_options *options, gpio_pin_t gpio);

#if !CONFIG_IDF_TARGET_ESP8266
  int gpio_intr_core();
#endif

#if GPIO_I2C_ENABLED
  /* i2c.cc */
  void gpio_i2c_intr_handler (const struct gpio_options *options, gpio_pins_t pins);

  int gpio_i2c_setup(const struct gpio_options *options);
  int gpio_i2c_setup_input(const struct gpio_options *options, gpio_pins_t pins);
  int gpio_i2c_get(const struct gpio_options *options, gpio_pins_t *pins);
  int gpio_i2c_setup_output(const struct gpio_options *options, gpio_pins_t pins);
  int gpio_i2c_set(const struct gpio_options *options, gpio_pins_t pins);

  /* gpio_i2c_pca54xx.c */
  int gpio_i2c_pca54xx_init(struct gpio_i2c_pca54xx_state *state);
  int gpio_i2c_pca54xx_setup(const struct gpio_options *options);
  int gpio_i2c_pca54xx_setup_input(const struct gpio_options *options, gpio_pins_t pins);
  int gpio_i2c_pca54xx_get(const struct gpio_options *options, gpio_pins_t *pins);
  int gpio_i2c_pca54xx_setup_output(const struct gpio_options *options, gpio_pins_t pins);
  int gpio_i2c_pca54xx_set(const struct gpio_options *options, gpio_pins_t pins);
#endif
