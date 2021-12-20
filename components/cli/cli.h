#pragma once

#include <cli.h>
#include <cmd.h>

struct cli {
  char *buf;
  size_t size;
  TickType_t timeout;

  struct cmdtab cmdtab;
  struct cmd_eval *cmd_eval;

  // false once cli_exit() called
  bool run;
};
