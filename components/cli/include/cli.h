#ifndef __CLI_H__
#define __CLI_H__

#include <cmd.h>

#include <FreeRTOS.h>
#include <stddef.h>

struct cli;

struct cli_options {
  // line buffer size, limits maximum command length
  size_t buf_size;

  // maximum number of CLI arguments per command
  size_t max_args;

  // read timeout for first character on each line, EOF on timeout
  TickType_t timeout;
};

/*
 * Allocate cli and start task for reading and evaluating CLI commands.
 */
int cli_init(struct cli **clip, const struct cmd *commands, struct cli_options options);

/*
 * Change the read timeout.
 */
void cli_set_timeout(struct cli *cli, TickType_t timeout);

/*
 * Run the CLI read-eval loop.
 *
 * Returns <0 on EOF, timeout, or other IO errors.
 */
int cli_main(struct cli *cli);

/*
 * Print help output.
 */
int cli_help(struct cli *cli);

/*
 * Terminate `cli_main()` loop.
 *
 * @return 0 if stopped, >0 if not running
 */
int cli_exit(struct cli *cli);

#endif
