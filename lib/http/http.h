#ifndef HTTP_H
#define HTTP_H

#include "stream.h"

#include <stddef.h>
#include <stdio.h>

struct http;

#define HTTP_VERSION "HTTP/1.0"

/* Maximum line length */
#define HTTP_LINE 1024

/* Maximum method length */
#define HTTP_METHOD_MAX 64

/* Maximum path length */
#define HTTP_PATH_MAX 1024

/* Maximum Host: header length */
#define HTTP_HOST_MAX 256

enum http_version {
    HTTP_10 = 0,    // default
    HTTP_11,
};

enum http_status {
    HTTP_OK                       = 200,
    HTTP_CREATED                  = 201,
    HTTP_FOUND                    = 301,
    HTTP_BAD_REQUEST              = 400,
    HTTP_FORBIDDEN                = 403,
    HTTP_NOT_FOUND                = 404,
    HTTP_METHOD_NOT_ALLOWED       = 405,
    HTTP_LENGTH_REQUIRED          = 411,
    HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
    HTTP_REQUEST_URI_TOO_LONG     = 414,
    HTTP_UNSUPPORTED_MEDIA_TYPE   = 415,
    HTTP_INTERNAL_SERVER_ERROR    = 500,
};

/*
 * Return a const char* with a textual reason for the given http status.
 */
const char * http_status_str (enum http_status status);

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
int http_write_response (struct http *http, const char *version, enum http_status status, const char *reason);

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
 * Send a HTTP request body from a file.
 *
 * content_length, if given, indicates the expected maximum size of the file to send, or until EOF otherwise.
 *
 * Returns 1 on (unexpected) EOF, <0 on error.
 */
int http_write_file (struct http *http, int fd, size_t content_length);

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
 * Returns 1 on EOF, <0 on error.
 */
int http_read_string (struct http *http, char **bufp, size_t len);

/*
 * Read the response body into FILE, or discard if -1.
 *
 * If content_length is given as 0, reads all content.
 */
int http_read_file (struct http *http, int fd, size_t content_length);

/*
 * Read response body chunks into FILE, or discard if -1
 */
int http_read_chunked_file (struct http *http, int fd);



/*
 * Release all associated resources.
 */
void http_destroy (struct http *http);

#endif
