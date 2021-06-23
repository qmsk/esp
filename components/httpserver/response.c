#include "response.h"
#include "httpserver/response.h"
#include "httpserver/request.h"

#include "http/http.h"
#include "http/http_file.h"
#include "logging.h"

int http_response_start (struct http_response *response, enum http_status status, const char *reason)
{
    enum http_version version = http_request_version(response->request);
    int err;

    LOG_DEBUG("reponse=%p version=%d status=%d reason=%s", response, version, status, reason ? reason : "");

    if (response->status) {
        LOG_WARN("attempting to re-send status: %u", status);
        return -1;
    }

    LOG_INFO("%s %u %s", http_version_str(version), status, reason ? reason : http_status_str(status));

    response->status = status;

    if ((err = http_write_response(response->http, version, status, reason))) {
        LOG_ERROR("failed to write response line");
        return err;
    }

    HTTP_HOOK_RETURN(response->hooks, HTTP_HOOK_RESPONSE, response, response, status, reason);
}

enum http_status http_response_get_status (struct http_response *response)
{
  LOG_DEBUG("response=%p: status=%d", response, response->status);

  return response->status;
}

int http_response_header (struct http_response *response, const char *name, const char *fmt, ...)
{
    int err;

    LOG_DEBUG("response=%p name=%s fmt=%s", response, name, fmt);

    if (!response->status) {
        LOG_WARN("attempting to send headers without status: %s", name);
        return -1;
    }

    if (response->headers) {
        LOG_ERROR("attempting to re-send headers");
        return -1;
    }

    HTTP_HOOK_CHECK_RETURN(response->hooks, HTTP_HOOK_RESPONSE_HEADER, response_header, response, name);

    LOG_INFO("\t%20s : %s", name, fmt);

    response->header = true;

    va_list args;
    va_start(args, fmt);
    err = http_write_headerv(response->http, name, fmt, args);
    va_end(args);

    if (err) {
        LOG_ERROR("failed to write response header line");
        return -1;
    }

    return 0;
}

int http_response_headers (struct http_response *response)
{
    LOG_DEBUG("response=%p", response);

    HTTP_HOOK_CHECK_RETURN(response->hooks, HTTP_HOOK_RESPONSE_HEADERS, response_headers, response);

    response->headers = true;

    if (http_write_headers(response->http)) {
        LOG_ERROR("failed to write end-of-headers");
        return -1;
    }

    return 0;
}

int http_response_sendfile (struct http_response *response, int fd, size_t content_length)
{
    int err;

    LOG_DEBUG("response=%p fd=%d content_length=%zu", response, fd, content_length);

    if (content_length) {
        LOG_DEBUG("using content-length");

        if ((err = http_response_header(response, "Content-Length", "%zu", content_length)))
            return err;
    } else {
        LOG_DEBUG("using connection close");

        response->close = true;

        if ((err = http_response_header(response, "Connection", "close")))
            return err;
    }

    // headers
    if ((err = http_response_headers(response))) {
        return err;
    }

    // body
    if (response->body) {
        LOG_WARN("attempting to re-send body");
        return -1;
    }

    response->body = true;

    if (http_sendfile(response->http, fd, content_length)) {
        LOG_ERROR("http_write_file");
        return -1;
    }

    return 0;
}

int http_response_print (struct http_response *response, const char *fmt, ...)
{
    va_list args;
    int err = 0;

    LOG_DEBUG("response=%p fmt=%s", response, fmt);

    if (!response->status) {
        LOG_WARN("attempting to send response body without status");
        return -1;
    }

    if (!response->headers) {
        // use chunked transfer-encoding for HTTP/1.1, and close connection for HTTP/1.0
        if (response->http11) {
            LOG_DEBUG("using chunked transfer-encoding");

            response->chunked = true;

            err |= http_response_header(response, "Transfer-Encoding", "chunked");
            err |= http_response_headers(response);
        } else {
            LOG_DEBUG("using connection close");

            response->close = true;

            err |= http_response_header(response, "Connection", "close");
            err |= http_response_headers(response);
        }
    }

    if (err)
        return err;

    // body
    response->body = true;

    va_start(args, fmt);
    if (response->chunked) {
        err = http_vprint_chunk(response->http, fmt, args);
    } else {
        err = http_vwrite(response->http, fmt, args);
    }
    va_end(args);

    if (err) {
        LOG_WARN("http_write");
        return err;
    }

    return 0;
}

int http_response_open (struct http_response *response, FILE **filep)
{
  int err = 0;

  LOG_DEBUG("response=%p", response);

  if (!response->status) {
      LOG_WARN("attempting to send response body without status");
      return -1;
  }

  // Set Connection: close
  LOG_DEBUG("using connection close");

  response->close = true;

  if ((err = http_response_header(response, "Connection", "close"))) {
    return err;
  }

  if ((err = http_response_headers(response))) {
    return err;
  }

  return http_file_open(response->http, 0, HTTP_STREAM_WRITE, filep);
}

int http_response_redirect (struct http_response *response, const char *host, const char *fmt, ...)
{
    char path[HTTP_RESPONSE_REDIRECT_PATH_MAX];
    int ret;
    va_list args;

    LOG_DEBUG("response=%p host=%s fmt=%s", response, host, fmt);

    va_start(args, fmt);
    ret = vsnprintf(path, sizeof(path), fmt, args);
    va_end(args);

    if (ret < 0) {
        LOG_ERROR("vsnprintf");
        return -1;
    } else if (ret >= sizeof(path)) {
        LOG_WARN("truncated redirect path: %d", ret);
        return -1;
    }

    // auto
    if (!host) {
        host = http_request_url(response->request)->host;
    }

    int err = 0;

    err |= http_response_start(response, 301, NULL);
    err |= http_response_header(response, "Location", "http://%s/%s", host, path);
    err |= http_response_headers(response);

    return err;
}

int http_response_error (struct http_response *response, enum http_status status, const char *reason, const char *detail)
{
    int err = 0;

    LOG_DEBUG("response=%p status=%d reason=%s detail=%s", response, status, reason, detail);

    if (!reason)
        reason = http_status_str(status);

    err |= http_response_start(response, status, reason);
    err |= http_response_header(response, "Content-Type", "text/html");
    err |= http_response_headers(response);
    err |= http_response_print(response, "<html><head><title>HTTP %d %s</title></head><body>\n", status, reason);
    err |= http_response_print(response, "<h1>HTTP %d %s</h1>", status, reason);

    if (detail)
        err |= http_response_print(response, "<p>%s</p>", detail);

    err |= http_response_print(response, "</body></html>\n");

    return err;
}

int http_response_close (struct http_response *response)
{
  int err = 0;

  LOG_DEBUG("response=%p", response);

  if (!response->status) {
    LOG_WARN("no response started");
    return -1;
  }

  // headers
  if (!response->headers) {
      // end-of-headers
      if (http_response_headers(response)) {
          LOG_WARN("http_response_headers");
          return -1;
      }
  }

  // entity
  if (response->chunked) {
      // end-of-chunks
      if ((err = http_write_chunks(response->http))) {
          LOG_WARN("failed to end response chunks");
          return -1;
      }
  }

  if (response->close) {
      return 1;

  } else {
      // persistent connection, keep alive
      return 0;
  }
}
