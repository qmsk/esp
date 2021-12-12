#include <spi_master.h>
#include "spi_master.h"
#include <logging.h>

#include <stdlib.h>

int spi_master_new(struct spi_master **spi_masterp, const struct spi_options options)
{
  struct spi_master *spi_master;
  int err;

  if (!(spi_master = calloc(1, sizeof(*spi_master)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = spi_master_init(spi_master, options))) {
    LOG_ERROR("spi_master_init");
    goto error;
  }

  if ((err = spi_master_intr_init(spi_master))) {
    LOG_ERROR("spi_master_intr_init");
    goto error;
  }

  *spi_masterp = spi_master;

  return 0;

error:
  free(spi_master);

  return err;
}
