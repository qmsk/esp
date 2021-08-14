#include <spi_master.h>
#include "spi_master.h"

#include <logging.h>

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp8266/gpio_struct.h>

enum spi_gpio spi_gpio_from_pin(unsigned pin)
{
  if (pin < 16) {
    return 1 << pin;
  } else {
    return 0;
  }
}

void spi_master_gpio_clear(struct spi_master *spi_master)
{
  if (spi_master->gpio & SPI_GPIO_CS_ACTIVE_HIGH) {
    GPIO.out_w1tc = (spi_master->gpio & SPI_GPIO_CS_MASK);
  } else {
    GPIO.out_w1ts = (spi_master->gpio & SPI_GPIO_CS_MASK);
  }
}

void spi_master_gpio_set(struct spi_master *spi_master, enum spi_gpio gpio)
{
  if (spi_master->gpio & SPI_GPIO_CS_ACTIVE_HIGH) {
    // first clear those not set
    GPIO.out_w1tc = (spi_master->gpio & SPI_GPIO_CS_MASK) & ~(gpio & SPI_GPIO_CS_MASK);
    GPIO.out_w1ts = (gpio & SPI_GPIO_CS_MASK);
  } else {
    // first clear those not set
    GPIO.out_w1ts = (spi_master->gpio & SPI_GPIO_CS_MASK) & ~(gpio & SPI_GPIO_CS_MASK);
    GPIO.out_w1tc = (gpio & SPI_GPIO_CS_MASK);
  }
}

int spi_master_gpio_init(struct spi_master *spi_master, enum spi_gpio gpio)
{
  gpio_config_t config = {
    .pin_bit_mask   = (gpio & SPI_GPIO_CS_MASK),
    .mode           = GPIO_MODE_OUTPUT,
  };
  esp_err_t err;

  spi_master->gpio = gpio;

  spi_master_gpio_clear(spi_master);

  if (!gpio) {
    // nothing to config
  } else if ((err = gpio_config(&config))) {
    LOG_ERROR("gpio_config: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
