#include "request.h"
#include "httpserver/request.h"

#include "http/http.h"
#include "http/http_file.h"
#include "logging.h"

#include <string.h>
#include <strings.h>

int http_request_read (struct http_request *request)
{
    const char *method, *path, *version;
    int err;

    LOG_DEBUG("request=%p", request);

    if (request->request) {
        LOG_WARN("re-reading request");
        return -1;
    }

    if ((err = http_read_request(request->http, &method, &path, &version))) {
        LOG_WARN("http_read_request");
        return err;
    }

    // mark as read
    request->request = true;

    LOG_INFO("%s %s %s", method, path, version);

    if (strlen(method) >= sizeof(request->method)) {
        LOG_WARN("method is too long: %zu", strlen(method));
        return 400;
    } else {
        strncpy(request->method, method, sizeof(request->method));
    }

    if (strlen(path) >= sizeof(request->pathbuf)) {
        LOG_WARN("path is too long: %zu", strlen(path));
        return 400;
    } else {
        strncpy(request->pathbuf, path, sizeof(request->pathbuf));
    }

    if ((err = url_parse(&request->url, request->pathbuf))) {
        LOG_WARN("url_parse: %s", request->pathbuf);
        return 400;
    }

    LOG_DEBUG("url: scheme=%p host=%p port=%p path=%p", request->url.scheme, request->url.host, request->url.port, request->url.path);

    if (request->url.scheme || request->url.host || request->url.port) {
        LOG_WARN("request url includes extra parts: scheme=%s host=%s port=%s",
            request->url.scheme ? request->url.scheme : "",
            request->url.host ? request->url.host : "",
            request->url.port ? request->url.port : ""
        );
        return 400;
    }

    if (strcasecmp(version, "HTTP/1.0") == 0) {
        request->version = HTTP_10;

        // implicit Connection: close
        request->close = true;

    } else if (strcasecmp(version, "HTTP/1.1") == 0) {
        request->version = HTTP_11;

    } else {
        LOG_WARN("unknown request version: %s", version);
        return 400;
    }

    if (strcasecmp(method, "GET") == 0) {
        // XXX: decoded in-place, stripping const
        request->get_query = (char *) request->url.query;
    } else if (strcasecmp(method, "POST") == 0) {
        request->post = true;
    }

    return 0;
}

const char *http_request_method (const struct http_request *request)
{
  return request->method;
}

const struct url *http_request_url(const struct http_request *request)
{
    return &request->url;
}

enum http_version http_request_version(const struct http_request *request)
{
  LOG_DEBUG("request=%p: version=%d", request, request->version);

  return request->version;
}

int http_request_query (struct http_request *request, const char **keyp, const char **valuep)
{
    LOG_DEBUG("request=%p: state=%s", request, request->get_query);

    return url_decode(&request->get_query, keyp, valuep);
}

int http_request_header (struct http_request *request, const char **namep, const char **valuep)
{
    int err;

    LOG_DEBUG("request=%p", request);

    if (!request->request) {
        LOG_WARN("premature read of request headers before request line");
        return -1;
    }

    if (request->headers) {
        LOG_WARN("request headers have already been read");
        return 1;
    }

    if ((err = http_read_header(request->http, namep, valuep)) < 0) {
        LOG_WARN("http_read_header");
        return err;
    }

    if (err == 1) {
        LOG_DEBUG("end of headers");
        request->headers = true;
        return 1;
    } else if (err) {
        LOG_WARN("http_read_header: %d", err);
        return err;
    }

    // mark as having read some headers
    request->header = true;

    LOG_INFO("\t%20s : %s", *namep, *valuep);

    if (strcasecmp(*namep, "Content-Length") == 0) {
        if (sscanf(*valuep, "%zu", &request->content_length) != 1) {
            LOG_WARN("invalid content_length: %s", *valuep);
            return 400;
        }

        LOG_DEBUG("content_length=%zu", request->content_length);

    } else if (strcasecmp(*namep, "Host") == 0) {
        if (strlen(*valuep) >= sizeof(request->hostbuf)) {
            LOG_WARN("host is too long: %zu", strlen(*valuep));
            return 400;
        } else {
            strncpy(request->hostbuf, *valuep, sizeof(request->hostbuf));
        }

        // TODO: parse :port?
        request->url.host = request->hostbuf;

    } else if (strcasecmp(*namep, "Connection") == 0) {
        if (strcasecmp(*valuep, "close") == 0) {
            LOG_DEBUG("using connection-close");

            request->close = true;

        } else if (strcasecmp(*valuep, "keep-alive") == 0) {
            /* Used by some HTTP/1.1 clients, apparently to request persistent connections.. */
            LOG_DEBUG("explicitly not using connection-close");

            request->close = false;

        } else {
            LOG_WARN("unknown connection header: %s", *valuep);
        }

    } else if (strcasecmp(*namep, "Content-Type") == 0) {
        if (strcasecmp(*valuep, "application/x-www-form-urlencoded") == 0) {
            LOG_DEBUG("request content is form data");

            request->content_form = true;
        }
    }

    return 0;
}

int http_request_form (struct http_request *request, const char **keyp, const char **valuep)
{
    LOG_DEBUG("request=%p", request);

    if (!request->headers) {
        LOG_ERROR("reading request form data before headers?");
        return -1;
    }

    if (request->post_form) {
        // continue using it

    } else if (request->body) {
        // have already read in body
        return 1;

    } else if (!request->content_length) {
        // no request body
        return 411;

    } else {
        // read in request body; either exactly content_length or to EOF
        if (http_read_string(request->http, &request->post_form, request->content_length)) {
            LOG_WARN("http_read_string");
            return -1;
        }

        // completed body
        request->body = true;
    }

    LOG_DEBUG("%s", request->post_form);

    // returns 1 after last param
    return url_decode(&request->post_form, keyp, valuep);
}

int http_request_param (struct http_request *request, const char **keyp, const char **valuep)
{
    LOG_DEBUG("request=%p", request);

    if (request->get_query)
        return http_request_query(request, keyp, valuep);

    if (!request->post)
        return 1;

    // yes, server_request_form also checks this, but we want to report this *before* 415
    if (!request->content_length)
        return 411; // Length Required

    if (!request->content_form)
        return 415; // Unsupported Media Type

    // returns 1 after last param
    return http_request_form(request, keyp, valuep);
}

int http_request_copy (struct http_request *request, int fd)
{
    int err;

    LOG_DEBUG("request=%p fd=%d", request, fd);

    if (!request->headers) {
        LOG_WARN("read request body without reading headers!?");
        return -1;
    }

    if (request->body) {
        LOG_WARN("re-reading request body...");
        return -1;
    }

    // TODO: Transfer-Encoding?
    if (!request->content_length) {
        LOG_DEBUG("no request body given");
        return 411;
    }

    if (((err = http_read_file(request->http, fd, request->content_length)))){
        LOG_WARN("http_read_file");
        return err;
    }

    // completed body
    request->body = true;

    return 0;
}

int http_request_close (struct http_request *request)
{
    int err;

    LOG_DEBUG("request=%p", request);

    // read remaining headers, in case they contain anything relevant for the error response
    if (!request->headers) {
        const char *header, *value;

        LOG_DEBUG("reading remaining headers...");

        // don't clobber err!
        while ((err = http_request_header(request, &header, &value)) != 1) {
          // ignore any 0 or >1 returns
          if (err < 0) {
            LOG_ERROR("http_request_header");
            return -1;
          }
        }
    }

    // body?
    // TODO: needs better logic for when a request contains a body?
    if (!request->body && request->content_length) {
        // force close, as pipelining will fail
        request->close = true;

        // we don't want to wait for the entire request body to upload before failing the request...
        LOG_WARN("ignored client request body, forcing connection close");
    }

    if (request->close) {
      LOG_DEBUG("connection close");

      // no more requests
      return 1;
    }

    return 0;
}
