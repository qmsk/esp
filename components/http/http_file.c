#undef __STRICT_ANSI__

#include "http.h"
#include "http/http.h"
#include "http/http_file.h"

#include "logging.h"

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int http_sendfile (struct http *http, int fd, size_t content_length)
{
    bool readall = !content_length;
    int err;

    while (content_length || readall) {
        size_t size = content_length;

        if ((err = stream_sendfile(http->write, fd, &size)) < 0) {
            LOG_WARN("stream_write_file %u", size);
            return err;
        }

        LOG_DEBUG("content_length=%u write=%u", content_length, size);

        if (err) {
            // EOF
            break;
        }

        // sanity-check
        if (content_length) {
            if (size > content_length) {
                LOG_ERROR("BUG: write=%u > content_length=%u", size, content_length);
                return -1;
            }

            content_length -= size;
        }
    }

    if (content_length) {
        LOG_WARN("premature EOF: %u", content_length);
        return 1;
    }

    return 0;
}

int http_read_file (struct http *http, int fd, size_t content_length)
{
    bool readall = !content_length;
    int err;

    while (content_length || readall) {
        LOG_DEBUG("content_length: %zu", content_length);

        size_t size = content_length;

        if (fd >= 0) {
            if ((err = stream_read_file(http->read, fd, &size)) < 0) {
                LOG_WARN("stream_read_file %zu", size);
                return err;
            }
        } else {
            char *ignore;

            if ((err = stream_read_ptr(http->read, &ignore, &size)) < 0) {
                LOG_WARN("stream_read_ptr %zu", size);
                return err;
            }
        }

        if (err) {
            // EOF
            LOG_DEBUG("eof");
            break;
        }

        // sanity-check
        if (content_length) {
            if (size > content_length) {
                LOG_ERROR("BUG: write=%zu > content_length=%zu", size, content_length);
                return -1;
            }

            content_length -= size;
        }
    }

    if (content_length) {
        LOG_WARN("premature EOF: %zu", content_length);
        return 1;
    }

    return 0;
}

int http_read_chunked_file (struct http *http, int fd)
{
    int err;

    // continue until http_read_chunk_header returns 1 for the last chunk
    while (http->chunk_size || !(err = http_read_chunk_header(http, &http->chunk_size))) {
        // maximum size to read
        size_t size = http->chunk_size;

        if (fd >= 0) {
            if ((err = stream_read_file(http->read, fd, &size)) < 0) {
                LOG_WARN("stream_read_file %zu", size);
                return err;
            }
        } else {
            char *ignore;

            if ((err = stream_read_ptr(http->read, &ignore, &size)) < 0) {
                LOG_WARN("stream_read_ptr %zu", size);
                return err;
            }
        }

        // mark how much of the chunk we have consumed
        http->chunk_size -= size;

        if (http->chunk_size) {
            // remaining chunk data
            LOG_DEBUG("%zu+%zu", size, http->chunk_size);

        } else {
            // end of chunk
            LOG_DEBUG("%zu", size);

            if ((err = http_read_chunk_footer(http)))
                return err;
        }
    }

    if (err < 0) {
        LOG_WARN("http_read_chunk_header");
        return err;
    } else if (err > 0) {
        LOG_WARN("http_read_chunk_header: EOF");
        return 1;
    }

    if ((err = http_read_chunks(http))) {
        LOG_WARN("http_read_chunks");
        return err;
    }

    return 0;
}

int http_file_read (void *cookie, char *buf, size_t len)
{
  struct http *http = cookie;
  int err;

  LOG_DEBUG("http=%p len=%u", http, len);

  if (http->read_content_length && len > *http->read_content_length) {
    LOG_DEBUG("limit len -> content_length=%u", *http->read_content_length);

    len = *http->read_content_length;
  }

  if (len == 0) {
    // EOF
    return 0;
  }

  if ((err = stream_read(http->read, buf, &len)) < 0) {
    return -1;
  } else if (err) {
    // EOF
    return 0;
  }

  if (*http->read_content_length) {
    *http->read_content_length -= len;
  }

  return len;
}

int http_file_write (void *cookie, const char *buf, size_t len)
{
  struct http *http = cookie;
  size_t size = len;
  int err;

  LOG_DEBUG("http=%p len=%u", http, len);

  if ((err = stream_write(http->write, buf, size)) < 0) {
    return -1;
  } else if (err) {
    // EOF
    return 0;
  } else {
    return size;
  }
}

int http_file_open (struct http *http, int flags, size_t *read_content_length, FILE **filep)
{
  const char *mode;
  cookie_io_functions_t functions = {};

  if (flags & HTTP_STREAM_READ) {
    mode = "r";

    http->read_content_length = read_content_length;
    functions.read = http_file_read;
  }

  if (flags & HTTP_STREAM_WRITE) {
    mode = "w";
    functions.write = http_file_write;
  }

  if ((flags & HTTP_STREAM_READ) && (flags & HTTP_STREAM_WRITE)) {
    mode = "w+";
  }

  LOG_DEBUG("http=%p content_length=%u: mode=%s", http, content_length, mode);

  if (!(*filep = fopencookie(http, mode, functions))) {
    LOG_WARN("fopencookie: %s", strerror(errno));
    return -1;
  }

  return 0;
}
