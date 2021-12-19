#include "log.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <logging.h>
#include <stdio_log.h>

#define STDIO_LINE_SIZE 512
#define STDIO_LOG_SIZE 1024

struct stdio_log *log_stderr;

static int putchar_stderr(int c)
{
  return fputc(c, stderr);
}

int init_log()
{
  int err;

  if ((err = stdio_log_new(&log_stderr, STDIO_LOG_SIZE))) {
    LOG_ERROR("stdio_log_new");
    return err;
  }

  if ((err = stdio_attach_stderr_log(log_stderr))) {
    LOG_ERROR("stdio_attach_stderr_log");
    return err;
  }

  // unbuffered logging via stderr
  setvbuf(stderr, NULL, _IONBF, 0);

  // redirect esp_log via stderr
  esp_log_set_putchar(putchar_stderr);

  return 0;
}

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
  { "show",     log_show_cmd,    .usage = "", .describe = "Show buffered log lines"  },
  { "tail",     log_tail_cmd,    .usage = "", .describe = "Show new log lines" },
  { "clear",    log_clear_cmd,   .usage = "", .describe = "Clear new log lines" },
  {}
};

const struct cmdtab log_cmdtab = {
  .commands = log_commands,
};
