#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <http/http_types.h>
#include <http/url.h>

struct http_request;

struct http_request_headers {
  /* Host header, or NULL */
  const char *host;

  /* Size of request entity, or zero */
  size_t content_length;

  /* Decoded Content-Type, or HTTP_CONTENT_TYPE_UNKNOWN */
  enum http_content_type content_type;
};

/*
 * Read the client request line.
 *
 * Returns 0 on success, <0 on internal error, 1 on EOF, http 4xx on client error.
 */
int http_request_read (struct http_request *request);

/*
 * Read request method.
 */
const char *http_request_method (const struct http_request *request);

/*
 * Read request URL.
 */
const struct url *http_request_url (const struct http_request *request);

/*
 * Read request version.
 */
enum http_version http_request_version (const struct http_request *request);

/*
 * Read request URL query param.
 *
 * NOTE: this modifies the `struct url` query path in-place.
 *
 * Returns 1 on end-of-params.
 */
int http_request_query (struct http_request *request, char **keyp, char **valuep);

/*
 * Read request header.
 *
 * Returns 1 on end-of-headers.
 */
int http_request_header (struct http_request *request, const char **name, const char **value);

/*
 * Read request headers. Optionally return pointer to decoded request headers.
 *
 * Returns <0 on error.
 */
int http_request_headers (struct http_request *request, const struct http_request_headers **headersp);

/*
 * Read request body form param.
 *
 * Returns 1 on end-of-params.
 */
int http_request_form (struct http_request *request, char **keyp, char **valuep);

/*
 * Read in request parameters, both GET and POST.
 *
 * Returns GET params first, then any POST <form> params.
 *
 * Returns 415 on a POST request with non-form Content-Type.
 * Returns 411 on a POST request with no Content-Length.
 *
 * Returns 1 on end-of-params, <0 on internal error, >0 on HTTP error, 0 on success.
 */
int http_request_param (struct http_request *request, char **keyp, char **valuep);

/*
 * Read request body from client into FILE.
 *
 * Returns >0 HTTP error if there was no request body.
 */
int http_request_copy (struct http_request *request, int fd);

/*
 * Finish reading any remaining request fields.
 *
 * Returns 1 if the connection should be closed, 0 if there may be more requests, <0 on error.
 */
int http_request_close (struct http_request *request);

/*
 * Returns 1 if the connection should be closed, 0 if there may be more requests.
 */
int http_request_closed (struct http_request *request);

#endif
