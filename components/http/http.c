#include "http/http.h"
#include "http.h"
#include "parse.h"
#include "util.h"

#include "logging.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


int http_create (struct http **httpp, struct stream *read, struct stream *write)
{
    struct http *http = NULL;

    if (!(http = calloc(1, sizeof(*http)))) {
        LOG_ERROR("calloc");
        goto error;
    }

    http->read = read;
    http->write = write;

    // ok
    *httpp = http;

    return 0;

error:
    if (http)
        free(http);

    return -1;
}

void http_reset (struct http *http)
{
  http->read_content_length = 0;
  http->chunk_size = 0;
}

/* Writing */
int http_write (struct http *http, const char *buf, size_t size)
{
    return stream_write(http->write, buf, size);
}

int http_vwrite (struct http *http, const char *fmt, va_list args)
{
    return stream_vprintf(http->write, fmt, args);
}

int http_writef (struct http *http, const char *fmt, ...)
{
    va_list args;
    int err;

    va_start(args, fmt);
    err = stream_vprintf(http->write, fmt, args);
    va_end(args);

    return err;
}

/*
 * Write a line to the HTTP socket.
 */
static int http_write_line (struct http *http, const char *fmt, ...)
{
    va_list args;
    int err;

    LOG_DEBUG("http=%p fmt=%s", http, fmt);

    va_start(args, fmt);
    err = stream_vprintf(http->write, fmt, args);
    va_end(args);

    if (err)
        return err;

    if ((err = stream_printf(http->write, "\r\n")))
        return err;

    return 0;
}



/* Reading */

/*
 * Read one line from the connection, returning a pointer to the (stripped) line in **linep.
 *
 * The returned pointer remains valid until the next http_read_line call, and may be modified.
 *
 * Returns 0 on success, <0 on error, >0 on EOF.
 */
static int http_read_line (struct http *http, char **linep)
{
    int err;

    if ((err = stream_read_line(http->read, linep)))
        return err;

    LOG_DEBUG("%s", *linep);

    return 0;
}

int http_parse_header (char *line, const char **headerp, const char **valuep)
{
    enum state { START, HEADER, SEP_PRE, SEP, SEP_POST, VALUE, END, FOLD_VALUE };
    struct parse parsing[] = {
        { START,        ' ',    FOLD_VALUE    },
        { START,        '\t',    FOLD_VALUE    },
        { START,        -1,        HEADER,        PARSE_KEEP        },

        { HEADER,        ' ',    SEP,        PARSE_STRING,    .parse_string = headerp        },
        { HEADER,        '\t',    SEP,        PARSE_STRING,    .parse_string = headerp        },
        { HEADER,        ':',    SEP_POST,    PARSE_STRING,    .parse_string = headerp        },

        { SEP,            ' ',    SEP            },
        { SEP,            '\t',    SEP            },
        { SEP,            ':',    SEP_POST    },

        { SEP_POST,        ' ',    SEP_POST    },
        { SEP_POST,        '\t',    SEP_POST    },
        { SEP_POST,        -1,        VALUE,        PARSE_KEEP         },

        { VALUE,        0,        END,        PARSE_STRING,    .parse_string = valuep        },

        /*For folded headers, we leave headerp as-is. */
        { FOLD_VALUE,    ' ',    FOLD_VALUE    },
        { FOLD_VALUE,    0,        END,        PARSE_STRING,    .parse_string = valuep        },

        { }
    };
    int err;

    // parse
    if ((err = parse(parsing, line, START)) != END)
        return 400;

    return 0;
}

/* Client request writing */
int http_write_request (struct http *http, const char *version, const char *method, const char *fmt, ...)
{
    va_list args;
    int err;

    if ((err = http_writef(http, "%s ", method)))
        return err;

    va_start(args, fmt);
    err = http_vwrite(http, fmt, args);
    va_end(args);

    if (err)
        return err;

    if ((err = http_writef(http, " %s\r\n", version)))
        return err;

    return 0;
}

int http_write_response (struct http *http, enum http_version version, enum http_status status, const char *reason)
{
    LOG_DEBUG("http=%p version=%d status=%d reason=%s", http, version, status, reason ? reason : "");

    return http_write_line(http, "%s %d %s", http_version_str(version), status, reason ? reason : http_status_str(status));
}

int http_write_headerv (struct http *http, const char *header, const char *fmt, va_list args)
{
    int err;

    if ((err = http_writef(http, "%s: ", header)))
        return err;

    if ((err = http_vwrite(http, fmt, args)))
        return err;

    if ((err = http_writef(http, "\r\n")))
        return err;

    return 0;
}

int http_write_header (struct http *http, const char *header, const char *fmt, ...)
{
    va_list args;
    int err;

    LOG_DEBUG("http=%p header=%s fmt=%s", http, header, fmt);

    va_start(args, fmt);
    err = http_write_headerv(http, header, fmt, args);
    va_end(args);

    return err;
}

int http_write_headers (struct http *http)
{
    LOG_DEBUG("http=%p", http);

    return http_write_line(http, "");
}

// 3.6.1 Chunked Transfer Coding
int http_write_chunk (struct http *http, const char *buf, size_t size)
{
    int err;

    LOG_DEBUG("%zu", size);

    if ((err = http_writef(http, "%zx\r\n", size)))
        return err;

    if ((err = stream_write(http->write, buf, size)))
        return err;

    if ((err = http_writef(http, "\r\n")))
        return err;

    return 0;
}

int http_vprint_chunk (struct http *http, const char *fmt, va_list inargs)
{
    va_list args;
    int err;

    // first, determine size
    int size;

    va_copy(args, inargs);
    size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (size < 0) {
        LOG_ERROR("snprintf");
        return -1;
    }

    if (!size) {
        LOG_WARN("emtpy chunk");
        return 0;
    }

    LOG_DEBUG("%d", size);

    // chunk header
    if ((err = http_writef(http, "%x\r\n", size)))
        return err;

    // print formatted chunk
    va_copy(args, inargs);
    err = stream_vprintf(http->write, fmt, args);
    va_end(args);

    if (err < 0)
        return err;

    // TODO: verify that vprintf'd size matches?

    // chunk trailer
    if ((err = http_writef(http, "\r\n")))
        return err;

    return 0;
}

int http_print_chunk (struct http *http, const char *fmt, ...)
{
    va_list args;
    int err;

    va_start(args, fmt);
    err = http_vprint_chunk(http, fmt, args);
    va_end(args);

    return err;
}

int http_write_chunks (struct http *http)
{
    // last-chunk + empty trailer + CRLF
    return http_writef(http, "0\r\n\r\n");
}

int http_parse_request (char *line, const char **methodp, const char **pathp, const char **versionp)
{
    enum state { START, METHOD, PATH, VERSION, END };
    struct parse parsing[] = {
        { START,      ' ',      -1                      },
        { START,      -1,       METHOD,     PARSE_KEEP  },

        { METHOD,     ' ',      PATH,       PARSE_STRING,    .parse_string = methodp  },
        { PATH,       ' ',      VERSION,    PARSE_STRING,    .parse_string = pathp    },
        { VERSION,    '\0',     END,        PARSE_STRING,    .parse_string = versionp },
        { }
    };
    int err;

    LOG_DEBUG("line=%s", line);

    // parse
    if ((err = parse(parsing, line, START)) != END) {
      LOG_DEBUG("parse: state=%d", err);

      return 400;
    }

    return 0;
}

int http_parse_response (char *line, const char **versionp, unsigned *statusp, const char **reasonp)
{
    enum state { START, VERSION, STATUS, REASON, END };
    struct parse parsing[] = {
        { START,     ' ',     -1            },
        { START,    -1,        VERSION,     PARSE_KEEP  },

        { VERSION,    ' ',    STATUS,       PARSE_STRING,     .parse_string = versionp  },
        { STATUS,    ' ',     REASON,       PARSE_UINT,       .parse_uint    = statusp  },
        { REASON,    '\0',    END,          PARSE_STRING,     .parse_string = reasonp   },
        { }
    };
    int err;

    LOG_DEBUG("line=%p", line);

    // parse
    if ((err = parse(parsing, line, START)) != END)
        return 400;

    return 0;
}

int http_read_request (struct http *http, const char **methodp, const char **pathp, const char **versionp)
{
    char *line;
    int err;

    if ((err = http_read_line(http, &line))) {
        LOG_WARN("http_read_line");
        return err;
    }

    if ((err = http_parse_request(line, methodp, pathp, versionp))) {
        LOG_WARN("http_parse_request");
        return err;
    }

    return 0;
}

int http_read_response (struct http *http, const char **versionp, unsigned *statusp, const char **reasonp)
{
    char *line;
    int err;

    if ((err = http_read_line(http, &line)))
        return err;

    return http_parse_response(line, versionp, statusp, reasonp);
}

int http_read_header (struct http *http, const char **headerp, const char **valuep)
{
    char *line;
    int err;

    if ((err = http_read_line(http, &line)) < 0)
        return err;

    if (err) {
        // XXX: interpret EOF as end-of-headers?
        LOG_WARN("eof during headers");
        return 1;
    }

    if (!*line) {
        LOG_DEBUG("end of headers");
        return 1;
    }

    return http_parse_header(line, headerp, valuep);
}

int http_read_string (struct http *http, char **bufp, size_t *lenp, char delim)
{
    size_t size = *lenp;
    int err;

    if ((err = stream_read_string(http->read, bufp, &size, delim))) {
      LOG_ERROR("stream_read_string");
      return err;
    }

    if (*lenp) {
      LOG_DEBUG("len=%u - size=%u", *lenp, size);

      *lenp -= size;
    }

    return 0;
}

int http_read_chunk_header (struct http *http, size_t *sizep)
{
    char *line;
    int err;

    // chunk header
    if ((err = http_read_line(http, &line)))
        return err;

    if (sscanf(line, "%zx", sizep) != 1) {
        LOG_ERROR("sscanf: %s", line);
        return -1;
    }

    if (!*sizep) {
        LOG_DEBUG("end-chunk");
        return 1;
    }

    return 0;
}

int http_read_chunk_footer (struct http *http)
{
    char *line;
    int err;

    if ((err = http_read_line(http, &line))) {
        LOG_WARN("http_read_line");
        return err;
    }

    if (*line) {
        LOG_WARN("trailing data after chunk");
        return -1;
    }

    return 0;
}

int http_read_chunked (struct http *http, char **bufp, size_t *sizep)
{
    int err;

    // continue to next chunk if current one is consumed, or just starting
    if (!http->chunk_size) {
        if ((err = http_read_chunk_header(http, &http->chunk_size)))
            return err;
    }

    // reading in at most chunk_size..
    *sizep = http->chunk_size;

    if ((err = stream_read_ptr(http->read, bufp, sizep)) < 0) {
        LOG_WARN("stream_read_ptr");
        return err;

    } else if (err) {
        LOG_WARN("eof");
        return -1;
    }

    // mark how much of the chunk we have consumed
    http->chunk_size -= *sizep;

    if (http->chunk_size) {
        // remaining chunk data
        LOG_DEBUG("%zu+%zu", *sizep, http->chunk_size);

    } else {
        // end of chunk
        LOG_DEBUG("%zu", *sizep);

        if ((err = http_read_chunk_footer(http)))
            return err;
    }

    return 0;
}

int http_read_chunks (struct http *http)
{
    char *line;
    int err;

    while (!(err = http_read_line(http, &line))) {
        if (!*line) {
            // terminates on empty line
            break;
        }

        // TODO: handle trailing headers as well?
        LOG_WARN("trailing header: %s", line);
    }

    return err;
}

void http_destroy (struct http *http)
{
    free(http);
}
