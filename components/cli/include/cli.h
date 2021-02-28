#ifndef __CLI_H__
#define __CLI_H__

#include "cmd.h"

#include <stddef.h>

struct cli;

/*
 * Allocate cli and start task for reading and evaluating CLI commands.
 */
int cli_init(struct cli **clip, const struct cmd *commands, size_t buf_size);

/*
 * Evaluate CLI command.
 */
int cli_cmd(struct cli *cli, char *line);

/*
 * Print help output.
 */
int cmd_help(struct cli *cli);

#endif
