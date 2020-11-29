#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <lib/http/types.h>

#include <stddef.h>

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
int http_response_file (struct http_response *response, int fd, size_t content_length);

/*
 * Send formatted data as part of the response.
 *
 * The response is sent without a Content-Length, and closed.
 */
int http_response_print (struct http_response *response, const char *fmt, ...)
    __attribute((format (printf, 2, 3)));

/*
 * Send a complete HTTP 301 redirect, using the given host and formatted path.
 *
 * The path should not include a leading /
 */
int http_response_redirect (struct http_response *response, const char *host, const char *fmt, ...);

/*
 * Send a complete (very basic) HTML-formatted HTTP error status response.
 */
int http_response_error (struct http_response *response, enum http_status status, const char *reason, const char *detail);

/*
 * Finish sending any incomplete response.
 *
 * Returns -1 if no response started. Returns 0 if connection remains open, 1 if connection closed.
 */
int http_response_close (struct http_response *response);

#endif
