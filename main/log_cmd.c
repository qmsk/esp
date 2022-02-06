#include "log.h"

#include <logging.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define STDIO_LINE_SIZE 512

static int read_log()
{
  char buf[STDIO_LINE_SIZE];

  while (fgets(buf, sizeof(buf), stderr)) {
    fputs(buf, stdout);
  }

  if (ferror(stderr)) {
    LOG_ERROR("stderr: %s", strerror(errno));
    return -1;
  }

  return 0;
}

int log_show_cmd(int argc, char **argv, void *ctx)
{
  if (fseek(stderr, 0, SEEK_SET)) {
    LOG_ERROR("fseek stderr: %s", strerror(errno));
    return -1;
  }

  return read_log();
}

int log_tail_cmd(int argc, char **argv, void *ctx)
{
  return read_log();
}

int log_clear_cmd(int argc, char **argv, void *ctx)
{
  if (fseek(stderr, 0, SEEK_END)) {
    LOG_ERROR("fseek stderr: %s", strerror(errno));
    return -1;
  }

  return 0;
}

const struct cmd log_commands[] = {
  { "show",     log_show_cmd,    .describe = "Show buffered log lines"  },
  { "tail",     log_tail_cmd,    .describe = "Show new log lines" },
  { "clear",    log_clear_cmd,   .describe = "Clear new log lines" },
  {}
};

const struct cmdtab log_cmdtab = {
  .commands = log_commands,
};
