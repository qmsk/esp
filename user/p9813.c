#include "p9813.h"
#include "p9813_config.h"
#include "user_cmd.h"
#include "spi.h"
#include "logging.h"

#include <stdlib.h>

static const enum SPI_Mode p9813_spi_mode = SPI_MODE_0;
static const enum SPI_Clock p9813_spi_clock = SPI_CLOCK_1MHZ;

struct __attribute__((packed)) p9813_packet {
  uint8_t control;
  uint8_t r, g, b;
};

void p9813_packet_set(struct p9813_packet *packet, uint8_t b, uint8_t g, uint8_t r)
{
  packet->control = 0xC0 | ((~b & 0xC0) >> 2) | ((~g & 0xC0) >> 4) | ((~r & 0xC0) >> 6);
  packet->b = b;
  packet->g = g;
  packet->r = r;
}

struct p9813 {
  struct spi *spi;

  unsigned count;

  struct p9813_packet *buf;
} p9813;

int p9813_init(struct p9813 *p9813, const struct p9813_config *config, struct spi *spi)
{
  struct SPI_MasterConfig spi_config = {
    .mode   = p9813_spi_mode,
    .clock  = p9813_spi_clock,
  };
  int err;

  p9813->spi = spi;
  p9813->count = config->count;

  if ((err = spi_setup(spi, spi_config))) {
    LOG_ERROR("spi_setup");
    return -1;
  }

  LOG_INFO("setup with spi(mode=%u clock=%u) p9813(count=%u)",
    spi_config.mode, spi_config.clock,
    config->count
  );

  if ((p9813->buf = malloc((1 + p9813->count + 1) * sizeof(*p9813->buf))) == NULL) {
    LOG_ERROR("malloc");
    return -1;
  }

  p9813->buf[0] = (struct p9813_packet){ 0, 0, 0, 0 }; // start

  for (unsigned i = 0; i < p9813->count; i++) {
    p9813->buf[1 + i] = (struct p9813_packet){ 0xFF, 0, 0, 0 }; // zero
  }

  p9813->buf[p9813->count + 1] = (struct p9813_packet){ 0, 0, 0, 0 }; // stop

  return 0;
}

int p9813_tx(struct p9813 *p9813)
{
  const char *ptr = (void *) p9813->buf;
  size_t len = (1 + p9813->count + 1) * sizeof(*p9813->buf);
  int ret;

  while (len > 0) {
    if ((ret = spi_write(p9813->spi, ptr, len)) < 0) {
      LOG_ERROR("spi_write");
      return ret;
    } else {
      ptr += ret;
      len -= ret;
    }
  }

  return 0;
}

int init_p9813 (const struct user_config *config)
{
  int err;

  if ((err = p9813_init(&p9813, &config->p9813, &spi1)))
    return err;

  return 0;
}

int p9813_cmd_set(int argc, char **argv, void *ctx)
{
  struct p9813 *p9813 = ctx;
  unsigned index, rgb;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &index)))
    return err;
  if ((err = cmd_arg_uint(argc, argv, 2, &rgb)))
    return err;

  if (index >= p9813->count) {
    LOG_ERROR("index out of bounds");
    return -CMD_ERR_ARGV;
  }

  p9813_packet_set(&p9813->buf[1 + index], (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 0) & 0xFF);

  return p9813_tx(p9813);
}

const struct cmd p9813_commands[] = {
  { "set",  p9813_cmd_set,  &p9813, .usage = "INDEX RGB", .describe = "Set values" },
  { }
};

const struct cmdtab p9813_cmdtab = {
  .commands = p9813_commands,
};
