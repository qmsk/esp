#include "cli.h"
#include <cli.h>
#include <logging.h>

#if CONFIG_NEWLIB_VFS_STDIO
#define HAVE_STDIO_FCNTL 1
#include <stdio_fcntl.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void cli_print_ctx(const struct cmdctx *ctx)
{
  if (ctx->parent) {
    cli_print_ctx(ctx->parent);
  }

  if (ctx->cmd) {
    printf(" %s", ctx->cmd->name);
  }
}

static int cli_print_help(const struct cmdtab *cmdtab, const struct cmdctx *parent)
{
  struct cmdctx ctx = {
    .parent = parent,
    .cmdtab = cmdtab,
  };

  for (const struct cmd *cmd = cmdtab->commands; cmd->name; cmd++) {
    ctx.cmd = cmd;

    if (cmd->func) {
      cli_print_ctx(&ctx);

      if (cmd->usage) {
        printf(" %s", cmd->usage);
      }

      if (cmd->describe) {
        printf(": %s\n", cmd->describe);
      } else {
        printf("\n");
      }
    }

    if (cmd->subcommands) {
      cli_print_help(cmd->subcommands, &ctx);
    }
  }

  return 0;
}

static int cli_cmd_error(const struct cmdctx *ctx, enum cmd_error err, const char *arg)
{
  printf("!");
  if (ctx->cmd || ctx->parent) {
    cli_print_ctx(ctx);
    printf(": ");
  } else {
    printf(" ");
  }
  printf("%s", cmd_strerror(err));
  if ((err & CMD_ERR_USAGE) && ctx->cmd && ctx->cmd->usage)
    printf(": %s\n", ctx->cmd->usage);
  else if (arg)
    printf(": %s\n", arg);
  else
    printf("\n");

  if (err == CMD_ERR_NOT_FOUND || err == CMD_ERR_MISSING_SUBCOMMAND) {
    cli_print_help(ctx->cmdtab, ctx->parent);
  }

  return 0;
}

// wait for initial command to open console
static int cli_open(struct cli *cli, TickType_t timeout)
{
  int c;

  // reset EOF state
  clearerr(stdin);

#if HAVE_STDIO_FCNTL
  if (fcntl(STDIN_FILENO, F_SET_READ_TIMEOUT, timeout) < 0) {
    LOG_WARN("fcntl stdin: %s", strerror(errno));
  }
#endif

  printf("! Use [ENTER] to open console\n");

  while ((c = fgetc(stdin)) != EOF) {
    if (c == '\r') {
      continue;
    } else if (c == '\n') {
      break;
    } else {
      printf("! Use [ENTER] to open console, ignoring <%02x>\n", c);
    }
  }

  if (ferror(stdin)) {
    LOG_ERROR("stdin: %s", strerror(errno));
    return -1;
  }

  if (feof(stdin)) {
    // timeout
    return 1;
  }

  return 0;
}

// process one line of input
static int cli_read(struct cli *cli)
{
  char *ptr = cli->buf;
  int c;

  // reset EOF state
  clearerr(stdin);

#if HAVE_STDIO_FCNTL
  // disable stdin timeout
  if (fcntl(STDIN_FILENO, F_SET_READ_TIMEOUT, 0) < 0) {
    LOG_WARN("fcntl stdin: %s", strerror(errno));
  }
#endif

  printf("> ");

  while ((c = fgetc(stdin)) != EOF) {
    #ifdef DEBUG
      if (isprint(c)) {
        LOG_DEBUG("%c", c);
      } else {
        LOG_DEBUG("%#02x", c);
      }
    #endif

    if (ptr >= cli->buf + cli->size) {
      LOG_WARN("line overflow");
      return 1;
    }

    // echo
    fputc(c, stdout);

    // handle CR or LF
    if (c == '\r') {
      fputc('\n', stdout);
    }

    if (c == '\b' && ptr > cli->buf) {
      // erase one char
      ptr--;

      // echo wipeout
      fprintf(stdout, " \b");

    } else if (c == '\r' || c == '\n') {
      *ptr = '\0';

      return 0;
    } else {
      // copy
      *ptr++ = c;
    }
  }

  if (ferror(stdin)) {
    LOG_ERROR("stdin: %s", strerror(errno));
    return -1;
  }

  // EOF
  LOG_WARN("EOF");
  return -1;
}

static int cli_eval(struct cli *cli)
{
  char *line = cli->buf;
  int err;

  if ((err = cmd_eval(cli->cmd_eval, &cli->cmdtab, line)) < 0) {
    LOG_ERROR("cmd_eval %s: %s", line, cmd_strerror(-err));
  } else if (err) {
    LOG_ERROR("cmd %s: %d", line, err);
  }

  return err;
}

int cli_main(struct cli *cli)
{
  int err;

  LOG_DEBUG("cli=%p", cli);

  if (!cli->timeout) {
    // skip open() step
  } else if ((err = cli_open(cli, cli->timeout)) < 0) {
    LOG_ERROR("cli_open");
    return err;
  } else if (err) {
    return 1;
  }

  for (cli->run = true; cli->run; ) {
    // read lines of input, ignore errors
    if ((err = cli_read(cli)) < 0) {
      LOG_ERROR("cli_read");
      return err;
    } else if (err) {
      continue;
    }

    // evaluatate
    if ((err = cli_eval(cli))) {
      continue;
    }
  }

  return 0;
}

int cli_init(struct cli **clip, const struct cmd *commands, struct cli_options options)
{
  struct cli *cli;
  int err = 0;

  if (!(cli = calloc(1, sizeof(*cli)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(cli->buf = malloc(options.buf_size))) {
    LOG_ERROR("malloc buf");
    goto error;
  }

  if ((err = cmd_eval_new(&cli->cmd_eval, options.max_args))) {
    LOG_ERROR("cmd_eval_new");
    goto error;
  }

  cli->size = options.buf_size;
  cli->timeout = options.timeout;
  cli->cmdtab = (struct cmdtab) {
    .commands      = commands,
    .error_handler = cli_cmd_error,
  };

  *clip = cli;

  return 0;

error:
  if (cli->buf) {
    free(cli->buf);
  }

  free(cli);

  return err;
}

void cli_set_timeout(struct cli *cli, TickType_t timeout)
{
  cli->timeout = timeout;
}

int cli_help(struct cli *cli)
{
  return cli_print_help(&cli->cmdtab, NULL);
}

int cli_exit(struct cli *cli)
{
  if (!cli->run) {
    return 1;
  }

  cli->run = false;

  return 0;
}
