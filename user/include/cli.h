#ifndef __USER_CLI_H__
#define __USER_CLI_H__

#include "user_config.h"
#include <cmd.h>

int init_cli(struct user_config *config, const struct cmdtab *commands);

void cli_putc(char c);
void cli_printf(const char *fmt, ...);

int cli_cmd_error(const struct cmdctx *ctx, enum cmd_error err, const char *cmd);
int cli_help_cmd(int argc, char **argv, void *ctx);

#endif
