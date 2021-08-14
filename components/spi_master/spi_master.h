#pragma once

#include <spi_master.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct spi_master {
  SemaphoreHandle_t mutex;
  SemaphoreHandle_t trans_done;

  enum spi_mode mode;
  enum spi_clock clock;
  enum spi_gpio gpio;
};

int spi_master_init(struct spi_master *spi_master, const struct spi_options options);

int spi_master_mode(struct spi_master *spi_master, enum spi_mode mode);
int spi_master_clock(struct spi_master *spi_master, enum spi_clock clock);
int spi_master_pins(struct spi_master *spi_master, enum spi_pins pins);

int spi_master_interrupt_init(struct spi_master *spi_master);
int spi_master_interrupt_wait_trans(struct spi_master *spi_master, TickType_t block_time);

int spi_master_gpio_init(struct spi_master *spi_master, enum spi_gpio gpio);
void spi_master_gpio_clear(struct spi_master *spi_master);
void spi_master_gpio_set(struct spi_master *spi_master, enum spi_gpio gpio);
