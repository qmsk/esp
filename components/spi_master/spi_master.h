#pragma once

#include <spi_master.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct spi_master {
  SemaphoreHandle_t mutex;
  SemaphoreHandle_t trans_done;

  enum spi_mode mode;
  enum spi_clock clock;
};

/* init.c */
int spi_master_init(struct spi_master *spi_master, const struct spi_options options);

int spi_master_mode(struct spi_master *spi_master, enum spi_mode mode);
int spi_master_clock(struct spi_master *spi_master, enum spi_clock clock);
int spi_master_pins(struct spi_master *spi_master, enum spi_pins pins);

/* intr.c */
int spi_master_intr_init(struct spi_master *spi_master);
int spi_master_intr_wait_trans_done(struct spi_master *spi_master, TickType_t block_time);

/* write.c */
void spi_master_trans_done_isr(struct spi_master *spi_master);
