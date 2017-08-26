#ifndef __CMD_H__
#define __CMD_H__

#define CMD_ARGS_MAX 8

typedef int(*cmd_func)(int argc, char **argv, void *ctx);

struct cmd {
  const char  *name;
  cmd_func    func;
  void        *ctx;
};

struct cmdtab {
  const struct cmd  *commands;
  void              *ctx;
};

enum cmd_error {
  CMD_ERR_OK          = 0,
  CMD_ERR_ARGS_MAX,
  CMD_ERR_NOT_FOUND,
  CMD_ERR_USAGE,
};

/** Subcommand handler: parse first argument as a sub-command
 *
 * @param ctx <cmd_tab>
 */
int cmd_handler(int argc, char **argv, void *ctx);

int cmd_eval(const struct cmdtab *cmdtab, char *line);

const char *cmd_strerror(enum cmd_error err);

#endif
