#include "cli.h"

#include <logging.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#if CONFIG_NEWLIB_VFS_STDIO
  #define HAVE_STDIO_FCNTL 1

  #include <stdio_fcntl.h>
#endif

enum state {
  ASCII,
  ESC,
  CSI,
  END,
};

static enum state cli_readline_ascii(struct cli *cli, char c)
{
  switch(c) {
    case '\b':
      if (cli->ptr > cli->buf) {
        // erase one char
        cli->ptr--;

        // echo wipeout
        fprintf(stdout, "\b \b");
      }

      return ASCII;

    case '\e':
      return ESC;

    case '\r':
      // echo CR -> CRLF
      fputc('\r', stdout);

      /* fall-through */

    case '\n':
      // echo
      fputc('\n', stdout);

      // end
      *cli->ptr = '\0';

      return END;

    default:
      // echo
      fputc(c, stdout);

      // copy
      *cli->ptr++ = c;

      return ASCII;
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
  CSI_CURSOR_UP = 'A',
};

static enum state cli_readline_csi_cursor_up(struct cli *cli)
{
  if (cli->ptr == cli->buf) {
    // restore previous line
    while (*cli->ptr) {
      char c = *cli->ptr++;

      fputc(c, stdout);
    }
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

  // start
  cli->ptr = cli->buf;

  printf("> ");

  while ((c = fgetc(stdin)) != EOF) {
    #ifdef DEBUG
      if (isprint(c)) {
        LOG_DEBUG("%c", c);
      } else {
        LOG_DEBUG("%#02x", c);
      }
    #endif

    if (cli->ptr >= cli->buf + cli->size) {
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

    if (state == END) {
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
