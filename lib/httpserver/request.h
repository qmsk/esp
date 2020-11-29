#ifndef REQUEST_H
#define REQUEST_H

#include "httpserver/response.h"

#include "http/http.h"
#include "http/types.h"
#include "http/url.h"

#include <stdbool.h>

/* Maximum method length */
#define HTTP_REQUEST_METHOD_MAX 64

/* Maximum path length */
#define HTTP_REQUEST_PATH_MAX 1024

/* Maximum Host: header length */
#define HTTP_REQUEST_HOST_MAX 256

struct http_request {
    struct http *http;
    struct http_response *response;

    /* Storage for request method field */
    char method[HTTP_REQUEST_METHOD_MAX];

    /* Storage for request path field; this is decoded into url and contains embedded NULs1 */
    char pathbuf[HTTP_REQUEST_PATH_MAX];

    /* Storage for request Host header */
    char hostbuf[HTTP_REQUEST_HOST_MAX];

    /* Decoded request URL, including query */
    struct url url;

    /* Decoded request version, or HTTP_10 if unknown. */
    enum http_version version;

    /* Client wishes to close connection */
    bool close;

    /* Size of request entity, or zero */
    size_t content_length;

    /* Content is application/x-www-form-urlencoded */
    bool content_form;

    /* Progress */
    bool request;
    bool header;
    bool headers;
    bool body;

    /* XXX: Decoding GET params in-place from url.query */
    char *get_query;

    /* Request is a POST request */
    bool post;

    /* Decoding POST params */
    char *post_form;
};

#endif
