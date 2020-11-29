#include "http.h"
#include "http/http.h"
#include "http/http_file.h"

#include "logging.h"

#include <stdio.h>
#include <stdbool.h>

int http_sendfile (struct http *http, int fd, size_t content_length)
{
    bool readall = !content_length;
    int err;

    while (content_length || readall) {
        size_t size = content_length;

        if ((err = stream_sendfile(http->write, fd, &size)) < 0) {
            LOG_WARN("stream_write_file %zu", size);
            return err;
        }

        LOG_DEBUG("content_length=%zu write=%zu", content_length, size);

        if (err) {
            // EOF
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

            if ((err = stream_read(http->read, &ignore, &size)) < 0) {
                LOG_WARN("stream_read %zu", size);
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

            if ((err = stream_read(http->read, &ignore, &size)) < 0) {
                LOG_WARN("stream_read %zu", size);
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
