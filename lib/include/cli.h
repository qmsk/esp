#ifndef __LIB_CLI_H__
#define __LIB_CLI_H__

#include "cmd.h"

void cli_putc(char c);
void cli_printf(const char *fmt, ...);

int cli_cmd_error(const struct cmdctx *ctx, enum cmd_error err, const char *cmd);
int cli_help_cmd(int argc, char **argv, void *ctx);

int cli_init(const struct cmdtab *commands);

#endif
