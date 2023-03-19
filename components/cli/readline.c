#include "cli.h"

#include <logging.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if CONFIG_NEWLIB_VFS_STDIO
  #define HAVE_STDIO_FCNTL 1

  #include <stdio_fcntl.h>
#endif

#define CLI_READLINE_PROMPT "> "

enum state {
  ASCII,
  ESC,
  CSI,
  END,
};

static enum state cli_readline_ascii_insert(struct cli *cli, char c)
{
  // insert
  for (char *p = cli->end; p > cli->ptr; p--) {
    *p = *(p - 1);
  }

  *cli->ptr++ = c;
  cli->end++;

  // echo
  fputc(c, stdout);

  for (char *p = cli->ptr; p < cli->end; p++) {
    fputc(*p, stdout);
  }

  // restore cursor position
  if (cli->ptr < cli->end) {
    fprintf(stdout, "\e[%dD", (cli->end - cli->ptr));
  }

  return ASCII;
}

static enum state cli_readline_ascii_remove(struct cli *cli, char c)
{
  if (cli->ptr > cli->buf) {
    // remove
    for (char *p = cli->ptr; p < cli->end; p++) {
      *(p - 1) = *p;
    }

    cli->ptr--;
    cli->end--;

    // echo wipeout
    fputc('\b', stdout);

    for (char *p = cli->ptr; p < cli->end; p++) {
      fputc(*p, stdout);
    }

    fputc(' ', stdout);

    // restore cursor position
    fprintf(stdout, "\e[%dD", (cli->end - cli->ptr + 1));
  }

  return ASCII;
}

static enum state cli_readline_ascii(struct cli *cli, char c)
{
  switch(c) {
    case '\b':
      return cli_readline_ascii_remove(cli, c);

    case '\e':
      return ESC;

    case '\r':
      // echo CR -> CRLF
      fputc('\r', stdout);

      /* fall-through */

    case '\n':
      // echo
      fputc('\n', stdout);

      return END;

    default:
      return cli_readline_ascii_insert(cli, c);
  }
}

static enum state cli_readline_esc(struct cli *cli, char c)
{
  switch(c) {
    case '[':
      return CSI;

    default:
      return ASCII;
  }
}

enum readline_csi_command {
  CSI_CURSOR_UP     = 'A',
  CSI_CURSOR_DOWN   = 'B',
  CSI_CURSOR_RIGHT  = 'C',
  CSI_CURSOR_LEFT   = 'D',
};

static enum state cli_readline_csi_cursor_up(struct cli *cli)
{
  if (cli->ptr == cli->buf && cli->prev_end) {
    // restore previous line
    while (cli->ptr < cli->prev_end) {
      char c = *cli->ptr++;

      fputc(c, stdout);
    }
  }

  return ASCII;
}

static enum state cli_readline_csi_cursor_down(struct cli *cli)
{
  if (cli->ptr != cli->buf) {
    // erase to end of line
    printf("\r%s\e[K", CLI_READLINE_PROMPT);

    cli->prev_end = cli->end;
    cli->ptr = cli->end = cli->buf;
  }

  return ASCII;
}

static enum state cli_readline_csi_cursor_left(struct cli *cli)
{
  if (cli->ptr > cli->buf) {
    printf("\e[D");

    cli->ptr--;
  }

  return ASCII;
}

static enum state cli_readline_csi_cursor_right(struct cli *cli)
{
  if (cli->ptr < cli->end) {
    printf("\e[C");

    cli->ptr++;
  }

  return ASCII;
}

static enum state cli_readline_csi(struct cli *cli, char c)
{
  switch(c) {
    case 0x20 ... 0x2F:  // intermediate
      return CSI;

    case 0x30 ... 0x3F:  // parameter
      return CSI;

    case CSI_CURSOR_UP:
      return cli_readline_csi_cursor_up(cli);

    case CSI_CURSOR_DOWN:
      return cli_readline_csi_cursor_down(cli);

    case CSI_CURSOR_RIGHT:
      return cli_readline_csi_cursor_right(cli);

    case CSI_CURSOR_LEFT:
      return cli_readline_csi_cursor_left(cli);

    default:
      return ASCII;
  }
}

int cli_readline(struct cli *cli)
{
  enum state state = ASCII;
  int c;

  // reset EOF state
  clearerr(stdin);

#if HAVE_STDIO_FCNTL
  // disable stdin timeout
  if (fcntl(STDIN_FILENO, F_SET_READ_TIMEOUT, 0) < 0) {
    LOG_WARN("fcntl stdin: %s", strerror(errno));
  }
#endif

  // history recall
  if (cli->end) {
    // undo NULs used by cmd_eval argv for history recall
    for (char *p = cli->buf; p < cli->end; p++) {
      if (*p == '\0') {
        *p = ' ';
      }
    }

    cli->prev_end = cli->end;
  }

  // start
  cli->ptr = cli->end = cli->buf;

  printf("%s", CLI_READLINE_PROMPT);

  while ((c = fgetc(stdin)) != EOF) {
    #ifdef DEBUG
      if (isprint(c)) {
        LOG_DEBUG("%c", c);
      } else {
        LOG_DEBUG("%#02x", c);
      }
    #endif

    if (cli->end >= cli->buf + cli->size) {
      LOG_WARN("line overflow");
      return 1;
    }

    switch(state) {
      case ASCII:
        state = cli_readline_ascii(cli, c);
        break;

      case ESC:
        state = cli_readline_esc(cli, c);
        break;

      case CSI:
        state = cli_readline_csi(cli, c);
        break;

      default:
        LOG_FATAL("state=%d", state);
    }

    if (cli->ptr > cli->end) {
      cli->end = cli->ptr;
    }

    if (cli->end >= cli->buf + cli->size) {
      LOG_WARN("line overflow");
      return 1;
    }

    if (state == END) {
      *cli->end = '\0';

      return 0;
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
