#include "i2s_test.h"

#include <i2s_out.h>
#include <logging.h>

struct i2s_out *i2s_out;

#define I2S_TEST_CLOCK I2S_DMA_CLOCK_3M2
#define I2S_TEST_BUFFER_SIZE 128

int init_i2s_test()
{
  int err;

  LOG_INFO("i2s_out_new... buffer_size=%u", I2S_TEST_BUFFER_SIZE);

  if ((err = i2s_out_new(&i2s_out, I2S_TEST_BUFFER_SIZE))) {
    LOG_ERROR("i2s_out_new");
    return err;
  }

  return 0;
}

int i2s_test_cmd_write(int argc, char **argv, void *ctx)
{
  struct i2s_out_options options = {
    .clock   = I2S_TEST_CLOCK,
  };
  int count = argc - 1;
  int err;

  if (!i2s_out && (err = init_i2s_test())) {
    LOG_ERROR("init_i2s_test");
    return err;
  }

  LOG_INFO("i2s_out_open... clock={%u, %u}",
    options.clock.clkm_div, options.clock.bck_div
  );

  if ((err = i2s_out_open(i2s_out, options))) {
    LOG_ERROR("i2s_out_open");
    return err;
  }

  for (int i = 0; i < count; i++) {
    unsigned value;
    int write;

    if ((err = cmd_arg_uint(argc, argv, i + 1, &value))) {
      goto error;
    }

    LOG_INFO("i2s_out_write... value=%#02x", value);

    if ((write = i2s_out_write(i2s_out, &value, sizeof(value))) < 0) {
      LOG_ERROR("i2s_out_write");
      err = write;
      goto error;
    } else {
      LOG_INFO("i2s_out_write: write=%d", write);
    }
  }

error:
  LOG_INFO("i2s_out_close...");

  if ((err = i2s_out_close(i2s_out))) {
    LOG_ERROR("i2s_out_close");
  }

  return err;
}

int i2s_test_cmd_count(int argc, char **argv, void *ctx)
{
  struct i2s_out_options options = {
    .clock   = I2S_TEST_CLOCK,
  };
  unsigned count;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &count))) {
    return err;
  }

  if (!i2s_out && (err = init_i2s_test())) {
    LOG_ERROR("init_i2s_test");
    return err;
  }

  LOG_INFO("i2s_out_open... clock={%u, %u}",
    options.clock.clkm_div, options.clock.bck_div
  );

  if ((err = i2s_out_open(i2s_out, options))) {
    LOG_ERROR("i2s_out_open");
    return err;
  }

  for (uint32_t i = 0; i < count; i++) {
    int write;

    LOG_INFO("i2s_out_write... value=%#02x", i);

    if ((write = i2s_out_write(i2s_out, &i, sizeof(i))) < 0) {
      LOG_ERROR("i2s_out_write");
      err = write;
      goto error;
    } else {
      LOG_INFO("i2s_out_write: write=%d", write);
    }
  }

error:
  LOG_INFO("i2s_out_close...");

  if ((err = i2s_out_close(i2s_out))) {
    LOG_ERROR("i2s_out_close");
  }

  return err;
}

const struct cmd i2s_test_commands[] = {
  { "write",   i2s_test_cmd_write,  .usage = "UINT32",           .describe = "Write multiple 32-bit output values" },
  { "count",   i2s_test_cmd_count,  .usage = "COUNT",            .describe = "Write incrementing 32-bit output values" },
  { }
};

const struct cmdtab i2s_test_cmdtab = {
  .commands = i2s_test_commands,
};
