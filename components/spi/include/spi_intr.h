#pragma once

enum spi_intr {
  SPI_INTR_SPI0,
  SPI_INTR_SPI1,
  SPI_INTR_I2S,

  SPI_INTR_MAX
};

typedef void (* spi_isr_t)(void *arg);

/**
 * Register shared SPI interrupt. Can be called multiple times by different modules.
 *
 * Any un-registered SPI0/SPI1/I2s interrupts will be cleared and disabled if they fire, and must be explicitly enabled after registering.
 */
void spi_intr_init();

/**
 * Register ISR handler for specific SPI interrupt.
 *
 * The interrupts must be enabled after installing the handler.
 */
void spi_intr_install(enum spi_intr intr, spi_isr_t isr, void *arg);
