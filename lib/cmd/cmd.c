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

static int cmd_call(const struct cmdtab *tab, const struct cmd *cmd, int argc, char **argv)
{
  void *ctx = cmd->ctx ? cmd->ctx : tab->ctx;

  return cmd->func(argc, argv, ctx);
}

int cmd_handler(int argc, char **argv, void *ctx)
{
  const struct cmdtab *cmdtab = ctx;
  const struct cmd *cmd;
  int err;

  if (argc < 2) {
    return -CMD_ERR_USAGE;
  } else if ((err = cmd_lookup(cmdtab, argv[1], &cmd))) {
    return err;
  }

  return cmd_call(cmdtab, cmd, argc - 1, argv + 1);
}

int cmd_eval(const struct cmdtab *cmdtab, char *line)
{
  int argc;
  char *argv[CMD_ARGS_MAX];
  const struct cmd *cmd;
  int ret;

  if ((ret = cmd_parse(line, argv, CMD_ARGS_MAX)) < 0) {
    return ret;
  } else if (ret == 0) {
    return 0;
  } else {
    argc = ret;
  }

  if ((ret = cmd_lookup(cmdtab, argv[0], &cmd))) {
    return ret;
  }

  return cmd_call(cmdtab, cmd, argc, argv);
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
    case CMD_ERR_USAGE:
      return "usage";
    default:
      return "<unknown>";
  }
}
