#ifndef URL_H
#define URL_H

#include <stdio.h>

struct url {
    const char *scheme;
    const char *host;
    const char *port;

    /* URL path *without* leading / */
    const char *path;

    /* Any ?... query string */
    const char *query;
};

/* Maximum supported url length */
#define URL_MAX 1024

struct urlbuf {
    char buf[URL_MAX];

    struct url url;
};

/*
 * Parse given url string, using given buffer, into an url struct.
 */
int urlbuf_parse (struct urlbuf *urlbuf, const char *url_string);

/*
 * Parse given url string (in-place) into an url struct.
 *
 * This does *not* NULL out url, so you can supply default values.
 */
int url_parse (struct url *url, char *url_string);

/*
 * Unquote any %HH or + -quoted url string in-place.
 */
int url_unquote (char *str);

/*
 * Decode a urlencoded key=value pair from the given url, updating the pointer to the next item, or NULL.
 *
 * This will also unquote the name/value.
 *
 * Returns 1 on end-of-keyvalues, 0 on name/value, -1 on error.
 */
int url_decode (char **queryp, char **namep, char **valuep);

/*
 * Write out URL to stream.
 */
void url_dump (const struct url *url, FILE *f);

#endif
