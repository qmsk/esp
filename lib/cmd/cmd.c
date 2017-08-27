#include "cmd.h"
#include <ctype.h>
#include <stddef.h>
#include <string.h>

static int cmd_parse(char *line, char **argv, int arg_max)
{
  char *ptr = line;
  const char *arg = NULL;
  int argc = 0;

  for (; *ptr; ptr++) {
    if (isspace((unsigned char) *ptr)) {
      *ptr = '\0';
      arg = NULL;
    } else if (!arg) {
      arg = argv[argc] = ptr;
      argc++;
    }

    if (argc >= arg_max) {
      return -CMD_ERR_ARGS_MAX;
    }
  }

  return argc;
}

static int cmd_lookup(const struct cmdtab *tab, const char *name, const struct cmd **cmdp)
{
  for (const struct cmd *cmd = tab->commands; cmd->name; cmd++) {
    if (strcmp(name, cmd->name) == 0) {
      *cmdp = cmd;
      return 0;
    }
  }

  return -CMD_ERR_NOT_FOUND;
}

static int cmd_call(const struct cmdtab *cmdtab, int argc, char **argv, const struct cmdctx *parent)
{
  struct cmdctx ctx = {
    .parent = parent,
    .cmdtab = cmdtab,
    .error_handler = cmdtab->error_handler ? cmdtab->error_handler : parent->error_handler,
  };
  int err;

  if (argc == 0) {
    err = -CMD_ERR_MISSING_SUBCOMMAND;
  } else if ((err = cmd_lookup(cmdtab, argv[0], &ctx.cmd))) {

  } else {
    if (ctx.cmd->subcommands) {
      err = cmd_call(ctx.cmd->subcommands, argc - 1, argv + 1, &ctx);
    } else if (ctx.cmd->func) {
      void *arg = ctx.cmd->arg ? ctx.cmd->arg : cmdtab->arg;

      err = ctx.cmd->func(argc, argv, arg);
    } else {
      err = -CMD_ERR_NOT_IMPLEMENTED;
    }
  }

  if (ctx.error_handler && err < 0) {
    return ctx.error_handler(&ctx, -err, argc ? argv[0] : NULL);
  } else {
    return err;
  }
}

int cmd_eval(const struct cmdtab *cmdtab, char *line)
{
  int argc;
  char *argv[CMD_ARGS_MAX];
  int ret;

  if ((ret = cmd_parse(line, argv, CMD_ARGS_MAX)) < 0) {
    return ret;
  } else if (ret == 0) {
    return 0;
  } else {
    argc = ret;
  }

  return cmd_call(cmdtab, argc, argv, NULL);
}

const char *cmd_strerror(enum cmd_error err)
{
  switch(err) {
    case CMD_ERR_OK:
      return "OK";
    case CMD_ERR_ARGS_MAX:
      return "maximum number of arguments exceeded";
    case CMD_ERR_NOT_FOUND:
      return "command not found";
    case CMD_ERR_NOT_IMPLEMENTED:
      return "command not implemented";
    case CMD_ERR_MISSING_SUBCOMMAND:
      return "missing subcommand";
    case CMD_ERR_USAGE:
      return "usage";
    case CMD_ERR_ARGUMENT:
      return "invalid argument";
    case CMD_ERR_FAILED:
      return "failed";
    case CMD_ERR_TIMEOUT:
      return "timeout";
    default:
      return "<unknown>";
  }
}
