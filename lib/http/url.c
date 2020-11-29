#include "parse.h"

#include "http/url.h"

#include "logging.h"

#include <stdbool.h>
#include <string.h>
#include <strings.h>

int urlbuf_parse (struct urlbuf *urlbuf, const char *url_string)
{
    memset(urlbuf, 0, sizeof(*urlbuf));

    if (strlen(url_string) > sizeof(urlbuf->buf)) {
        LOG_ERROR("url is too long: %zu", strlen(url_string));
        return -1;
    }

    strncpy(urlbuf->buf, url_string, sizeof(urlbuf->buf));
    urlbuf->buf[sizeof(urlbuf->buf) - 1] = '\0';

    LOG_DEBUG("parse: %s", urlbuf->buf);

    if (url_parse(&urlbuf->url, urlbuf->buf)) {
        LOG_WARN("invalid url: %s", url_string);
        return 1;
    }

    return 0;
}

int url_parse (struct url *url, char *buf)
{
    enum state {
        START = 0,
        START_SEP,
        SCHEME,
        SCHEME_SEP,
        HOST_PRE,
        HOST,
        HOST_IPV6,
        HOST_POST,
        PORT,
        PATH,
        QUERY,
    };
    struct parse parsing[] = {
        { START,        '/',        START_SEP   },
        { START,        ':',        SCHEME,     PARSE_SKIP                    },
        { START,        '[',        HOST_IPV6   },
        { START,        0,          HOST,       PARSE_STRING,               .parse_string = &url->host            },

        { START_SEP,    '/',        HOST_PRE    },
        { START_SEP,    '?',        QUERY,      PARSE_STRING,               .parse_string = &url->path          },
        { START_SEP,    0,          PATH,       PARSE_STRING,               .parse_string = &url->path          },
        { START_SEP,    -1,         PATH,       PARSE_STRING | PARSE_KEEP,  .parse_string = &url->path          },

        { SCHEME,       '/',        SCHEME_SEP, PARSE_STRING,               .parse_string = &url->scheme        },
        { SCHEME,       -1,         PORT,       PARSE_STRING | PARSE_KEEP,  .parse_string = &url->host            },

        { SCHEME_SEP,   '/',        HOST_PRE    },

        { HOST_PRE,     '[',        HOST_IPV6 },
        { HOST_PRE,     '/',        PATH,       PARSE_STRING,               .parse_string = &url->host          },
        { HOST_PRE,     -1,         HOST,       PARSE_KEEP  },

        { HOST,         ':',        PORT,       PARSE_STRING,               .parse_string = &url->host          },
        { HOST,         '/',        PATH,       PARSE_STRING,               .parse_string = &url->host          },
        { HOST,         0,          HOST,       PARSE_STRING,               .parse_string = &url->host          },

        // XXX: too lax
        { HOST_IPV6,    ']',        HOST_POST,  PARSE_STRING,               .parse_string = &url->host          },

        { HOST_POST,    ':',        PORT        },
        { HOST_POST,    '/',        PATH        },
        { HOST_POST,    0,          HOST_POST   },
        { HOST_POST,    -1,         -1          },

        { PORT,         '/',        PATH,       PARSE_STRING,               .parse_string = &url->port          },
        { PORT,         0,          PORT,       PARSE_STRING,               .parse_string = &url->port          },

        { PATH,         '?',        QUERY,      PARSE_STRING,               .parse_string = &url->path          },
        { PATH,         0,          PATH,       PARSE_STRING,               .parse_string = &url->path          },

        { QUERY,        0,          QUERY,      PARSE_STRING,               .parse_string = &url->query         },

        { }
    };
    int state;

    if ((state = parse(parsing, buf, START)) <= 0) {
        return -1;
    }

    return 0;
}

int url_unquote_hex (const char **inp, char **outp)
{
    // read in first two chars of hh, with the short-circuited && taking care of any NULs
    char buf[3] = { };

    if (!((buf[0] = *(*inp)++) && (buf[1] = *(*inp)++))) {
        // premature end-of-string
        return 1;
    }

    // decode
    unsigned int value;

    if (sscanf(buf, "%x", &value) != 1)
        // non-hex escape
        return 1;

    if (!value)
        // inserting NUL?
        return 1;

    *(*outp)++ = value;

    return 0;
}

int url_unquote (char *str)
{
    const char *in = str;
    char *out = str;
    char c;

    while ((c = *in++)) {
        switch (c) {
            case '+':
                *out++ = ' ';
                break;

            case '%':
                if (url_unquote_hex(&in, &out))
                    return -1;
                break;

            default:
                *out++ = c;
                break;
        }
    }

    // terminate
    *out = '\0';

    return 0;
}

int url_decode (char **queryp, const char **namep, const char **valuep)
{
    char *query;
    char *name = NULL, *value = NULL, *next = NULL;

    if (!(query = *queryp) || !(*query))
        // set to NULL on last token
        return 1;

    enum {
        NAME,
        VALUE,
        NEXT,
        END,
    } state;
    struct parse parsing[] = {
        { NAME,         '=',        VALUE,      PARSE_STRING,               .parse_string = (const char **) &name },
        { NAME,         '&',        NEXT,       PARSE_STRING,               .parse_string = (const char **) &name },
        { NAME,         0,          END,        PARSE_STRING,               .parse_string = (const char **) &name },

        { VALUE,        '&',        NEXT,       PARSE_STRING,               .parse_string = (const char **) &value },
        { VALUE,        0,          END,        PARSE_STRING,               .parse_string = (const char **) &value },

        { NEXT,         0,          NEXT,       PARSE_STRING,               .parse_string = (const char **) &next },

        { }
    };

    state = parse(parsing, query, NAME);

    // unquote?
    if (name)
        url_unquote(name);

    if (value)
        url_unquote(value);

    // return
    *queryp = next;
    *namep = name;
    *valuep = value;

    switch (state) {
        case NEXT:
            return 0;

        case END:
            // we already know that there is no following token.. but this is more convenient
            return 0;

        default:
            return -1;
    }
}

void url_dump (const struct url *url, FILE *f)
{
    if (url->scheme) {
        fprintf(f, "%s:", url->scheme);
    }

    if (url->host) {
        fprintf(f, "//%s", url->host);

    }

    if (url->port) {
        fprintf(f, ":%s", url->port);
    }

    if (url->path) {
        fprintf(f, "/%s", url->path);
    }
}
