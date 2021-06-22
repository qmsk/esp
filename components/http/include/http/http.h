#ifndef HTTP_H
#define HTTP_H

#include "stream.h"
#include "http_types.h"

#include <stddef.h>
#include <stdio.h>

struct http;

/*
 * Create a new HTTP connect using the given IO streams.
 *
 * Does not take ownership of the streams; they must be destroyed by the caller after http_destroy().
 */
int http_create (struct http **httpp, struct stream *read, struct stream *write);

/*
 * Write data from memory, as part of the message body.
 *
 * XXX: ensure that the use of stream_write does not place limits on maximum length.
 */
int http_write (struct http *http, const char *buf, size_t size);

// XXX: should be http_print
/*
 * Write formatted data, as part of the message body.
 */
int http_vwrite (struct http *http, const char *fmt, va_list args);
int http_writef (struct http *http, const char *fmt, ...)
    __attribute((format (printf, 2, 3)));

/*
 * Send a HTTP request line.
 */
int http_write_request (struct http *http, const char *version, const char *method, const char *fmt, ...)
    __attribute((format (printf, 4, 5)));

/*
 * Send a HTTP response line.
 *
 * Reason can be passed as NULL if status is a recognized status code.
 */
int http_write_response (struct http *http, enum http_version version, enum http_status status, const char *reason);

/*
 * Send one HTTP header.
 */
int http_write_headerv (struct http *http, const char *header, const char *fmt, va_list args);
int http_write_header (struct http *http, const char *header, const char *fmt, ...)
    __attribute((format (printf, 3, 4)));

/*
 * End the HTTP headers.
 */
int http_write_headers (struct http *http);

/*
 * Send a HTTP/1.1 'Transfer-Encoding: chunked' entity chunk.
 */
int http_write_chunk (struct http *http, const char *buf, size_t size);

/*
 * Send a HTTP/1.1 'Transfer-Encoding: chunked' entity chunk based on string formatting.
 */
int http_vprint_chunk (struct http *http, const char *fmt, va_list args);
int http_print_chunk (struct http *http, const char *fmt, ...)
    __attribute((format (printf, 2, 3)));

/*
 * Send a HTTP/1.1 'Transfer-Encoding: chunked' last-chunk and empty trailer.
 */
int http_write_chunks (struct http *http);




/*
 * Read a HTTP request.
 */
int http_read_request (struct http *http, const char **methodp, const char **pathp, const char **versionp);

/*
 * Read a HTTP response.
 */
int http_read_response (struct http *http, const char **versionp, unsigned *statusp, const char **reasonp);

/*
 * Read next header as {*headerp}: {*valuep}.
 *
 * In case of a folded header, *headerp is left as-is.
 *
 * Returns 1 on end-of-headers, 0 on header, <0 on error.
 */
int http_read_header (struct http *http, const char **headerp, const char **valuep);

/*
 * Read the response body as a NUL-terminated string.
 *
 * The maximum size to read should be given in len, or 0 to read to EOF.
 *
 * NOTE: does not support chunked read.
 *
 * Returns 1 on EOF, <0 on error.
 */
int http_read_string (struct http *http, char **bufp, size_t len);

/*
 * Read chunked header indicating chunk size.
 *
 * The last chunk has a zero size.
 *
 * Returns 1 on EOF, <0 on error, 0 on success.
 */
int http_read_chunk_header (struct http *http, size_t *sizep);

/*
 * Read chunked footer indicating end of chunk.
 *
 * Returns 1 on EOF, <0 on error, 0 on success.
 */
int http_read_chunk_footer (struct http *http);

/*
 * Read in a chunked response.
 *
 * Note that this does not necessarily read in an entire chunk at a time, but will return partial chunks.
 *
 * Returns 0 on success with *sizep updated, 1 on end-of-chunks, -1 on error or invalid chunk.
 */
int http_read_chunked (struct http *http, char **bufp, size_t *sizep);

/*
 * Read end-of-chunks trailer.
 */
int http_read_chunks (struct http *http);

/*
 * Release all associated resources.
 */
void http_destroy (struct http *http);

#endif
