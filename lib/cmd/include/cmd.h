#ifndef __CMD_H__
#define __CMD_H__

#define CMD_ARGS_MAX 8

struct cmd;
struct cmdtab;
struct cmdctx;
enum cmd_error;

typedef int(*cmd_func)(int argc, char **argv, void *arg);
typedef int(*cmdtab_error_func)(const struct cmdctx *ctx, enum cmd_error err, const char *arg);

struct cmd {
  const char      *name;
  cmd_func         func;
  void            *arg;
  const char      *usage;
  const char      *describe;
  const struct cmdtab   *subcommands; // sub-commands
};

struct cmdtab {
  const struct cmd  *commands;
  void              *arg;
  cmdtab_error_func error_handler;
};

struct cmdctx {
  const struct cmdctx *parent;
  const struct cmdtab *cmdtab;
  const struct cmd *cmd;
  cmdtab_error_func error_handler;
};

enum cmd_error {
  CMD_ERR_OK          = 0,
  CMD_ERR_ARGS_MAX,
  CMD_ERR_NOT_FOUND,
  CMD_ERR_NOT_IMPLEMENTED,
  CMD_ERR_MISSING_SUBCOMMAND,
  CMD_ERR_USAGE,
  CMD_ERR_ARGUMENT,
  CMD_ERR_FAILED,
  CMD_ERR_TIMEOUT,
};

int cmd_eval(const struct cmdtab *cmdtab, char *line);

const char *cmd_strerror(enum cmd_error err);

#endif
