#include "spi.h"
#include "user_cmd.h"
#include "logging.h"

#include <drivers/spi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct spi {
  enum SPI spi;
} spi1;

int spi_init(struct spi *spi, enum SPI port)
{
  spi->spi = port;

  return 0;
}

int spi_setup(struct spi *spi, struct SPI_MasterConfig spi_config)
{
  SPI_SetupMaster(spi->spi, spi_config);

  return 0;
}

int spi_send(struct spi *spi, const struct SPI_Operation *op)
{
  LOG_INFO("spi=%d cmd=%u:%x addr=%u:%x dummy=%u data=%u", spi->spi,
    op->command_bits, op->command,
    op->address_bits, op->address,
    op->dummy_cycles,
    op->data_bits
  );

  SPI_Send(spi->spi, op);

  return 0;
}

int spi_write(struct spi *spi, const void *buf, size_t len)
{
  struct SPI_Operation op = {
      .data_bits = len * 8,
  };

  if (len > sizeof(op.data_buf)) {
    LOG_ERROR("len > %u", sizeof(op.data_buf));
    return -1;
  }

  memcpy(op.data_buf, buf, len);

  LOG_INFO("spi=%d len=%u", spi->spi, len);

  SPI_Send(spi->spi, &op);

  return len;
}

int init_spi()
{
  struct SPI_MasterConfig spi_config = {};
  int err;

  if ((err = spi_init(&spi1, SPI_1)))
    return err;

  if ((err = spi_setup(&spi1, spi_config)))
    return err;

  return 0;
}

int spi_cmd_setup(int argc, char **argv, void *ctx)
{
  struct spi *spi = ctx;
  unsigned mode, clock;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &mode)))
    return err;
  if ((err = cmd_arg_uint(argc, argv, 2, &clock)))
    return err;

  struct SPI_MasterConfig spi_config = {
    .mode = mode,
    .clock = clock,
  };

  return spi_setup(spi, spi_config);
}

int spi_cmd_send(int argc, char **argv, void *ctx)
{
  struct spi *spi = ctx;
  struct SPI_Operation op;
  int value;

  if (argc <= 1) {
    return -CMD_ERR_ARGC;
  } else if (sscanf(argv[1], "%u:%i", &op.command_bits, &value) <= 0) {
    return -CMD_ERR_ARGV;
  } else {
    op.command = value;
  }

  if (argc <= 2) {
    return -CMD_ERR_ARGC;
  } else if (sscanf(argv[2], "%u:%x", &op.address_bits, &value) <= 0) {
    return -CMD_ERR_ARGV;
  } else {
    op.address = value;
  }

  if (argc <= 3) {
    return -CMD_ERR_ARGC;
  } else if (sscanf(argv[3], "%u", &op.dummy_cycles) <= 0) {
    return -CMD_ERR_ARGV;
  } else {

  }

  if (argc <= 4) {
    return -CMD_ERR_ARGC;
  } else if (sscanf(argv[4], "%u:%64s", &op.data_bits, (char *) op.data_buf) <= 0) {
    return -CMD_ERR_ARGV;
  } else {

  }

  return spi_send(spi, &op);
}

int spi_cmd_write(int argc, char **argv, void *ctx)
{
  struct spi *spi = ctx;
  unsigned count = argc - 1;
  uint8_t *buf;
  int err;

  if ((buf = malloc(count)) == NULL) {
    LOG_ERROR("malloc");
    return -CMD_ERR;
  }

  for (unsigned i = 0; i < count; i++) {
    if ((err = cmd_arg_uint8(argc, argv, 1 + i, &buf[i])))
      goto error;
  }

  if ((err = spi_write(spi, buf, count)) < 0) {
    goto error;
  } else {
    err = 0;
  }

error:
  free(buf);

  return err;
}

const struct cmd spi_commands[] = {
  { "setup",  spi_cmd_setup,  &spi1, .usage = "MODE CLOCK-DIV", .describe = "Setup SPI master" },
  { "send",   spi_cmd_send,   &spi1, .usage = "BITS:COMMAND BITS:HEX-ADDRESS BITS:STRING-DATA", .describe = "Send op" },
  { "write",  spi_cmd_write,  &spi1, .usage = "[BYTE [...]]", .describe = "Send bytes" },
  { }
};

const struct cmdtab spi_cmdtab = {
  .commands = spi_commands,
};
