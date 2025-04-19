#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <http/http_types.h>

#include <stddef.h>
#include <stdio.h>

struct http_response;

/*
 * Send response to client request.
 */
int http_response_start (struct http_response *response, enum http_status status, const char *reason);

/*
 * Get HTTP status as sent.
 *
 * @return 0 if no response started
 */
enum http_status http_response_get_status (struct http_response *response);

/*
 * Send response header.
 */
int http_response_header (struct http_response *response, const char *name, const char *fmt, ...)
    __attribute((format (printf, 3, 4)));

/*
 * End response headers.
 */
 int http_response_headers (struct http_response *response);

/*
 * Send response body from file.
 */
int http_response_sendfile (struct http_response *response, int fd, size_t content_length);

/*
 * Send formatted data as part of the response.
 *
 * The response is sent without a Content-Length, and closed.
 */
int http_response_vprintf (struct http_response *response, const char *fmt, va_list args)
__attribute((format (printf, 2, 0)));

/*
 * Send formatted data as part of the response.
 *
 * The response is sent without a Content-Length, and closed.
 */
int http_response_printf (struct http_response *response, const char *fmt, ...)
    __attribute((format (printf, 2, 3)));

/*
 * Return stdio FILE for writing to response.
 *
 * The response is sent without a Content-Length, and closed.
 */
int http_response_open (struct http_response *response, FILE **filep);

/*
 * Send a complete HTTP 301 redirect, using the given host and formatted path.
 *
 * The path should not include a leading /
 */
int http_response_redirect (struct http_response *response, const char *host, const char *fmt, ...)
    __attribute((format (printf, 3, 4)));

/*
 * Send a complete (very basic) text-formatted HTTP error status response.
 *
 * Returns status > 0, can be used to terminate handler.
 */
int http_response_error (struct http_response *response, enum http_status status, const char *reason, const char *fmt, ...)
    __attribute((format (printf, 4, 5)));

/*
 * Finish sending any incomplete response.
 *
 * Returns -1 if no response started. Returns 0 if connection remains open, 1 if connection closed.
 */
int http_response_close (struct http_response *response);

#endif
