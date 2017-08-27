#include "cmd.h"
#include <stdio.h>

int cmd_arg_str(int argc, char **argv, int arg, const char **strp)
{
  if (argc < arg)
    return -CMD_ERR_ARGC;

  *strp = argv[arg];

  return 0;
}

int cmd_arg_int(int argc, char **argv, int arg, int *intp)
{
  if (argc < arg)
    return -CMD_ERR_ARGC;
  else if (sscanf(argv[arg], "%d", intp) <= 0)
    return -CMD_ERR_ARGV;

  return 0;
}

int cmd_arg_uint(int argc, char **argv, int arg, unsigned *uintp)
{
  if (argc < arg)
    return -CMD_ERR_ARGC;
  else if (sscanf(argv[arg], "%u", uintp) <= 0)
    return -CMD_ERR_ARGV;

  return 0;
}

int cmd_arg_uint8(int argc, char **argv, int arg, uint8_t *uint8p)
{
  unsigned uint;

  if (argc < arg)
    return -CMD_ERR_ARGC;
  else if (sscanf(argv[arg], "%u", &uint) <= 0)
    return -CMD_ERR_ARGV;
  else if (uint > UINT8_MAX)
    return -CMD_ERR_ARGV;
  else
    *uint8p = (uint8_t) uint;

  return 0;

}
