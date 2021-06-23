#ifndef RESPONSE_H
#define RESPONSE_H

#include "httpserver/request.h"
#include "httpserver/hooks.h"
#include "http/http.h"

#include <stdbool.h>

#define HTTP_RESPONSE_REDIRECT_PATH_MAX 512

struct http_response {
    struct http *http;
    struct http_request *request;
    const struct http_hooks *hooks;

    /* Does the client support HTTP/1.1? */
    bool http11; // set from http_request

    /* The response status has been sent */
    unsigned status;

    /* One or many response headers have been sent */
    bool header;

    /* Response end-of-headers have been sent */
    bool headers;

    /* Response body has been started */
    bool body;

    /* Response entity body is being sent using chunked transfer encoding; must be ended */
    bool chunked;

    /* Close connection after response; may be determined by client or by response method */
    /* TODO: Some HTTP/1.0 clients may send a Connection:keep-alive request header, whereupon
     * it may be possible to have !request.http11 && !response.close, in which situation we should
     * be returning a 'Connection: keep-alive' response header...
     */
    bool close; // set from http_request
};

#endif
