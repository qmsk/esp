#pragma once

#include <cli.h>
#include <cmd.h>

struct cli {
  char *buf, *ptr;
  size_t size;
  TickType_t timeout;

  struct cmdtab cmdtab;
  struct cmd_eval *cmd_eval;

  // false once cli_exit() called
  bool run;
};

// process one line of input
int cli_readline(struct cli *cli);
