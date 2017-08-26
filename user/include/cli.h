#ifndef __USER_CLI_H__
#define __USER_CLI_H__

#include "user_config.h"

typedef int(*cli_command_handler)(int argc, char **argv, void *ctx);

struct cli_command {
  const char *command;
  cli_command_handler handler;
  void *ctx;
};

int init_cli(struct user_config *config, const struct cli_command *commands);

#endif
