#include "log.h"

#include <logging.h>
#include <stdio_log.h>

#include <stdio.h>

#define STDIO_LOG_SIZE 1024

struct stdio_log *log_stderr;

static int vprintf_stderr(const char * fmt, va_list args)
{
  return vfprintf(stderr, fmt, args);
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
  esp_log_set_vprintf(vprintf_stderr);

  return 0;
}
