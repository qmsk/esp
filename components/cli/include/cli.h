#ifndef __CLI_H__
#define __CLI_H__

#include <cmd.h>

#include <stddef.h>

struct cli;

/*
 * Allocate cli and start task for reading and evaluating CLI commands.
 */
int cli_init(struct cli **clip, const struct cmd *commands, size_t buf_size);

/*
 * Run the CLI read-eval loop.
 */
void cli_main(struct cli *cli);

/*
 * Print help output.
 */
int cmd_help(struct cli *cli);

#endif
