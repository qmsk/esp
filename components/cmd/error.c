#include "cmd.h"

const char *cmd_strerror(enum cmd_error err)
{
  switch(err) {
    case CMD_ERR_OK:
      return "OK";
    case CMD_ERR:
      return "error";
    case CMD_ERR_ARGC:
      return "invalid argument count";
    case CMD_ERR_ARGV:
      return "invalid argument value";

    case CMD_ERR_ALLOC:
      return "memory allocation failed";
    case CMD_ERR_ARGS_MAX:
      return "maximum number of arguments exceeded";
    case CMD_ERR_NOT_FOUND:
      return "command not found";
    case CMD_ERR_NOT_IMPLEMENTED:
      return "command not implemented";
    case CMD_ERR_MISSING_SUBCOMMAND:
      return "missing subcommand";
    case CMD_ERR_TIMEOUT:
      return "timeout";
    default:
      return "<unknown>";
  }
}
