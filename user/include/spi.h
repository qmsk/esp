#ifndef __USER_SPI_H__
#define __USER_SPI_H__

#include <stddef.h>
#include <drivers/spi_config.h>

int init_spi();

struct spi;

extern struct spi spi1;

int spi_setup(struct spi *spi, struct SPI_MasterConfig spi_config);

/*
 * Sends under 64-bytes are atomic.
 *
 * @param buf start of buf
 * @param len size of buf in bytes
 * @return number of bytes sent, <0 on error
 */
int spi_write(struct spi *spi, const void *buf, size_t len);

#endif
