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

    // XXX: decoded in-place, stripping const
    request->query = (char *) request->url.query;

    if (strcasecmp(method, "GET") == 0) {

    } else if (strcasecmp(method, "POST") == 0) {
        request->post = true;
    }

    HTTP_HOOK_RETURN(request->hooks, HTTP_HOOK_REQUEST, request, request, method, path, version);
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

int http_request_query (struct http_request *request, char **keyp, char **valuep)
{
    LOG_DEBUG("request=%p: query=%s", request, request->query);

    return url_decode(&request->query, keyp, valuep);
}

int http_request_header (struct http_request *request, const char **namep, const char **valuep)
{
    int err;

    LOG_DEBUG("request=%p", request);

    if (!request->request) {
        LOG_WARN("premature read of request headers before request line");
        return -1;
    }

    if (request->headers_done) {
        LOG_WARN("request headers have already been read");
        return 1;
    }

    if ((err = http_read_header(request->http, namep, valuep)) < 0) {
        LOG_WARN("http_read_header");
        return err;
    }

    if (err == 1) {
        LOG_DEBUG("end of headers");
        request->headers_done = true;
        HTTP_HOOK_CHECK_RETURN(request->hooks, HTTP_HOOK_REQUEST_HEADERS, request_headers, request);
        return 1;
    } else if (err) {
        LOG_WARN("http_read_header: %d", err);
        return err;
    }

    // mark as having read some headers
    request->header = true;

    LOG_INFO("\t%20s : %s", *namep, *valuep);

    if (strcasecmp(*namep, "Content-Length") == 0) {
        if (sscanf(*valuep, "%zu", &request->headers.content_length) != 1) {
            LOG_WARN("invalid content_length: %s", *valuep);
            return 400;
        }

        LOG_DEBUG("content_length=%zu", request->headers.content_length);

    } else if (strcasecmp(*namep, "Host") == 0) {
        if (strlen(*valuep) >= sizeof(request->hostbuf)) {
            LOG_WARN("host is too long: %zu", strlen(*valuep));
            return 400;
        } else {
            strncpy(request->hostbuf, *valuep, sizeof(request->hostbuf));
        }

        // TODO: parse :port?
        request->headers.host = request->hostbuf;
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
        request->headers.content_type = http_content_type_parse(*valuep);

        if (request->headers.content_type == HTTP_CONTENT_TYPE_UNKNOWN) {
            LOG_DEBUG("unknown content-type: %s", *valuep);
        } else {
            LOG_DEBUG("decoded content-type: %s", *valuep);
        }
    }

    HTTP_HOOK_RETURN(request->hooks, HTTP_HOOK_REQUEST_HEADER, request_header, request, *namep, *valuep);
}

int http_request_headers (struct http_request *request, const struct http_request_headers **headersp)
{
    int err;

    // read remaining headers
    if (!request->headers_done) {
        const char *header, *value;

        while ((err = http_request_header(request, &header, &value)) != 1) {
            if (err) {
                LOG_ERROR("http_request_header");
                return err;
            }
        }
    }

    if (!request->headers_hook) {
      request->headers_hook = true;

      HTTP_HOOK_CHECK_RETURN(request->hooks, HTTP_HOOK_REQUEST_RESPONSE, request_response, request, request->response);
    }

    if (headersp) {
      *headersp = &request->headers;
    }

    return 0;
}

int http_request_form (struct http_request *request, char **keyp, char **valuep)
{
    char *ptr;

    if (!request->headers_done) {
        LOG_ERROR("reading request form data before headers?");
        return -1;
    }

    if (request->body_form) {
        // continue using it

    } else if (request->body) {
        // have already read in body
        return 1;

    } else if (!request->headers.content_length) {
        // no request body
        return 411;

    } else {
      // start reading
      request->body = true;
      request->body_form = true;
    }

    LOG_DEBUG("request=%p: content_length=%u", request, request->headers.content_length);

    if (!request->headers.content_length) {
      // end-of-request
      return 1;
    }

    // read up to next form delimiter, or up to request EOF
    if (http_read_string(request->http, &ptr, &request->headers.content_length, '&')) {
        LOG_WARN("http_read_string");
        return -1;
    }

    LOG_DEBUG("request=%p: ptr=%s content_length=%u", request, ptr, request->headers.content_length);

    // returns if empty
    return url_decode(&ptr, keyp, valuep);
}

int http_request_param (struct http_request *request, char **keyp, char **valuep)
{
    LOG_DEBUG("request=%p", request);

    if (request->query)
        return http_request_query(request, keyp, valuep);

    if (!request->post)
        return 1;

    // yes, server_request_form also checks this, but we want to report this *before* 415
    if (!request->headers.content_length)
        return 411; // Length Required

    if (request->headers.content_type != HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED)
        return 415; // Unsupported Media Type

    // returns 1 after last param
    return http_request_form(request, keyp, valuep);
}

int http_request_copy (struct http_request *request, int fd)
{
    int err;

    LOG_DEBUG("request=%p fd=%d", request, fd);

    if (!request->headers_done) {
        LOG_WARN("read request body without reading headers!?");
        return -1;
    }

    if (request->body) {
        LOG_WARN("re-reading request body...");
        return -1;
    }

    // TODO: Transfer-Encoding, Connection: close?
    if (!request->headers.content_length) {
        LOG_WARN("no request Content-Length");
        return 411;
    }

    if (((err = http_read_file(request->http, fd, request->headers.content_length)))){
        LOG_WARN("http_read_file");
        return err;
    }

    // completed body
    request->body = true;

    return 0;
}

int http_request_open (struct http_request *request, FILE **filep)
{
  int err;

  LOG_DEBUG("request=%p", request);

  if (!request->headers_done) {
      LOG_WARN("read request body without reading headers");
      return -1;
  }

  if (request->body) {
      LOG_WARN("re-reading request body");
      return -1;
  }

  // TODO: Transfer-Encoding, Connection: close?
  if (!request->headers.content_length) {
    LOG_WARN("no request Content-Length");
    return 411;
  }

  if ((err = http_file_open(request->http, HTTP_STREAM_READ, &request->headers.content_length, filep))) {
    LOG_ERROR("http_file_open");
    return err;
  }

  // XXX: unknown if body completed
  //request->body = true;

  return 0;
}

int http_request_close (struct http_request *request)
{
    int err;

    LOG_DEBUG("request=%p", request);

    // read remaining headers, in case they contain anything relevant for the error response
    if (!request->headers_done) {
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

        // supress response hook
        request->headers_hook = true;
    }

    // body?
    // TODO: needs better logic for when a request contains a body?
    if (!request->body && request->headers.content_length) {
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

int http_request_closed (struct http_request *request)
{
  if (request->close) {
    return 1; // no more requests
  }

  return 0;
}
