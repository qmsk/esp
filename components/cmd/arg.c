#include "cmd.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

int cmd_arg_str(int argc, char **argv, int arg, const char **strp)
{
  if (arg >= argc)
    return -CMD_ERR_ARGC;

  *strp = argv[arg];

  return 0;
}

int cmd_arg_strncpy(int argc, char **argv, int arg, char *buf, size_t len)
{
  if (arg >= argc)
    return -CMD_ERR_ARGC;

  if (strlen(argv[arg]) > len)
    return -CMD_ERR_ARGV;

  strncpy(buf, argv[arg], len);

  return 0;
}

int cmd_arg_int(int argc, char **argv, int arg, int *p)
{
  int value;

  if (arg >= argc)
    return -CMD_ERR_ARGC;
  else if (sscanf(argv[arg], "%i", &value) <= 0)
    return -CMD_ERR_ARGV;
  else
    *p = value;

  return 0;
}

int cmd_arg_uint(int argc, char **argv, int arg, unsigned *p)
{
  long int value;

  if (arg >= argc)
    return -CMD_ERR_ARGC;
  else if (sscanf(argv[arg], "%li", &value) <= 0)
    return -CMD_ERR_ARGV;
  else if (value < 0 || value > UINT_MAX)
    return -CMD_ERR_ARGV;
  else
    *p = (unsigned) value;

  return 0;
}

int cmd_arg_uint8(int argc, char **argv, int arg, uint8_t *p)
{
  int value;

  if (arg >= argc)
    return -CMD_ERR_ARGC;
  else if (sscanf(argv[arg], "%i", &value) <= 0)
    return -CMD_ERR_ARGV;
  else if (value < 0 || value > UINT8_MAX)
    return -CMD_ERR_ARGV;
  else
    *p = (uint8_t) value;

  return 0;

}
