#include "http/stream.h"

#include "logging.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Read buffer start, for reading new data into the stream.
 */
static inline char * stream_readbuf_ptr (struct stream *stream)
{
    return stream->buf + stream->length;
}

/*
 * Read buffer size, for reading new data into the stream.
 */
static inline size_t stream_readbuf_size (struct stream *stream)
{
    return stream->size - stream->length;
}

/*
 * Write buffer start, for reading old data from the stream.
 */
static inline char * stream_writebuf_ptr (struct stream *stream)
{
    return stream->buf + stream->offset;
}

/*
 * Write buffer size, for reading old data from the stream.
 */
static inline size_t stream_writebuf_size (struct stream *stream)
{
    return stream->length - stream->offset;
}

/*
 * Write buffer end, for reading old data from the stream.
 */
static inline char * stream_writebuf_end (struct stream *stream)
{
    return stream->buf + stream->length;
}

static int stream_init (const struct stream_type *type, struct stream *stream, size_t size, void *ctx)
{
    // buffer
    if (!(stream->buf = malloc(size))) {
        LOG_ERROR("malloc %zu", size);
        return -1;
    }

    stream->type = type;
    stream->size = size;
    stream->length = 0;
    stream->offset = 0;
    stream->ctx = ctx;

    return 0;
}

int stream_create (const struct stream_type *type, struct stream **streamp, size_t size, void *ctx)
{
    struct stream *stream;

    if (!(stream = calloc(1, sizeof(*stream)))) {
        LOG_ERROR("calloc");
        return -1;
    }

    if (stream_init(type, stream, size, ctx))
        goto error;

    *streamp = stream;
    return 0;

error:
    free(stream);
    return -1;
}

/*
 * Mark given readbuf bytes as valid.
 */
inline static void stream_read_mark (struct stream *stream, size_t size)
{
    // XXX: assert lenth < size
    stream->length += size;
}

/*
 * Mark given writebuf bytes as consumed.
 */
inline static void stream_write_mark (struct stream *stream, size_t size)
{
    // XXX: assert offset < length
    stream->offset += size;
}

/*
 * Mark writebuf as consumed.
 */
inline static void stream_write_consume (struct stream *stream)
{
    stream->offset = stream->length;
}

/*
 * Clean out marked write buffer, making more room for the read buffer.
 *
 * Returns -1 if the stream buffer is full.
 */
int _stream_clear (struct stream *stream)
{
    if (!(stream->offset || stream->length < stream->size)) {
        LOG_WARN("stream write buffer is full, no room for read");
        return -1;
    }

    memmove(stream->buf, stream->buf + stream->offset, stream->length - stream->offset);

    stream->length -= stream->offset;
    stream->offset = 0;

    return 0;
}

/*
 * Read directly from the stream into the given external buffer.
 *
 * The stream must not have any data buffered.
 * Returns bytes read in *sizep.
 *
 * Returns 0 on success, 1 on EOF, <0 on error.
 */
int _stream_read_direct (struct stream *stream, char *buf, size_t *sizep)
{
    int err;

    if ((err = stream->type->read(buf, sizep, stream->ctx)) < 0) {
        LOG_WARN("stream-write: %s", strerror(errno));
        return err;
    } else if (err && *sizep) {
        LOG_DEBUG("eof");
        return 1;
    } else if (err) {
        LOG_DEBUG("timeout");
        return -1;
    }

    return 0;
}

/*
 * Read into stream from fd. There must be room available for the read buffer.
 *
 * This function is guaranteed to make progress.
 *
 * Returns 1 on EOF. There may still be data in the buffer.
 *
 * Returns -1 on error or timeout.
 */
int _stream_read (struct stream *stream)
{
    int err;

    // fill up
    size_t size = stream_readbuf_size(stream);

    if ((err = stream->type->read(stream_readbuf_ptr(stream), &size, stream->ctx)) < 0) {
        LOG_WARN("stream-read: %s", strerror(errno));
        return err;
    }

    if (!size) {
        LOG_DEBUG("eof");
        return 1;
    }

    if (err) {
        LOG_DEBUG("timeout");
        return -1;
    }

    stream_read_mark(stream, size);

    return 0;
}

/*
 * Put one char into the stream.
 */
int _stream_append (struct stream *stream, char c)
{
    if (!stream_readbuf_size(stream))
        return -1;

    *stream_readbuf_ptr(stream) = c;

    stream_read_mark(stream, 1);

    return 0;
}

/*
 * Insert one char into the stream.
 */
int _stream_insert (struct stream *stream, char c, size_t i)
{
    if (i > stream_writebuf_size(stream)) {
        LOG_DEBUG("insert %zu out of bounds %zu", i, stream_writebuf_size(stream));
        return -1;
    }

    if (!stream_readbuf_size(stream)) {
        LOG_DEBUG("insert into full buffer");
        return -1;
    }

    // shift one place right
    memmove(stream_writebuf_ptr(stream) + i + 1, stream_writebuf_ptr(stream) + i, stream_writebuf_size(stream) - i);

    // insert
    *(stream_writebuf_ptr(stream) + i) = c;

    // mark
    stream_read_mark(stream, 1);

    return 0;
}

/*
 * Write directly from the given external buffer to the stream.
 *
 * Guaranteed to write the entire buffer contents on success.
 *
 * Returns 0 on success, 1 on EOF, <0 on error.
 */
int _stream_write_direct (struct stream *stream, const char *buf, size_t size)
{
    int err;

    while (size) {
        size_t len = size;

        if ((err = stream->type->write(buf, &len, stream->ctx)) < 0) {
            LOG_WARN("stream-write: %s", strerror(errno));
            return err;
        }

        if (!len) {
            LOG_DEBUG("eof");
            return 1;
        }

        if (err) {
            LOG_DEBUG("timeout");
            return -1;
        }

        buf += len;
        size -= len;
    }

    return 0;
}

/*
 * Write some data out from the stream.
 *
 * Guaranteed to always make some progress on success, but may not necessarily write out the entire buffer.
 *
 * Returns 0 on success, 1 on EOF, <0 on error.
 */
int _stream_write (struct stream *stream)
{
    size_t size = stream_writebuf_size(stream);
    int err;

    if (!size) {
        // XXX
        LOG_WARN("write without anything to write");
        return 0;
    }

    if ((err = stream->type->write(stream_writebuf_ptr(stream), &size, stream->ctx) < 0)) {
        return err;
    }

    if (!size) {
        LOG_DEBUG("eof");
        return 1;
    }

    if (err) {
        LOG_DEBUG("timeout");
        return -1;
    }

    stream_write_mark(stream, size);

    return 0;
}

int stream_read (struct stream *stream, char *buf, size_t *sizep)
{
  size_t size = *sizep, len = 0;
  int err;

  if (stream_writebuf_size(stream) == 0) {
    // no buffered data to consume
    len = 0;
  } else if (stream_writebuf_size(stream) < size) {
    // consume all buffered data
    len = stream_writebuf_size(stream);
  } else {
    // consume partial data
    len = size;
  }

  // first consume any buffered data
  if (len > 0) {
    memcpy(buf, stream_writebuf_ptr(stream), len);

    stream_write_mark(stream, len);

    buf += len;
    size -= len;
  }

  // fill in any remaining data directly
  if (size > 0) {
    if ((err = _stream_read_direct(stream, buf, &size)) < 0) {
      LOG_WARN("_stream_read_direct");
      return err;
    }

    buf += size;
    len += size;
  }

  *sizep = len;

  return 0;
}

int stream_read_ptr (struct stream *stream, char **bufp, size_t *sizep)
{
    int err;

    // make room if needed
    if ((err = _stream_clear(stream)))
        return err;

    // until we have the request amount of data, or any data, or EOF
    while (stream->length < stream->offset + *sizep) {
        if ((err = _stream_read(stream)) < 0)
            return err;

        if (err)
            break;
    }

    *bufp = stream->buf + stream->offset;

    if (*sizep && stream->offset + *sizep < stream->length) {
        // have full requested size

    } else if (stream->length > stream->offset) {
        // have partial requested size, or however much available
        *sizep = stream->length - stream->offset;
    } else {
        // EOF
        return 1;
    }

    // consumed
    stream->offset += *sizep;

    return 0;
}

int stream_read_line (struct stream *stream, char **linep)
{
    char *c;
    int err;

    // make room if needed
    if ((err = _stream_clear(stream)))
        return err;

    while (true) {
        // scan for \r\n
        for (c = stream_writebuf_ptr(stream); c < stream_writebuf_end(stream); c++) {
            if (*c == '\r') {
                *c = '\0';
            } else if (*c == '\n') {
                *c = '\0';
                goto out;
            }
        }

        // needs moar bytez in mah buffers
        // XXX: should we return the last line on EOF, or expect a trailing \r\n?
        if ((err = _stream_read(stream)))
            return err;
    }

out:
    // start of line
    *linep = stream_writebuf_ptr(stream);

    // past end of line
    stream_write_mark(stream, c - stream_writebuf_ptr(stream) + 1);

    return 0;
}

int stream_read_string (struct stream *stream, char **strp, size_t len)
{
    int err;

    // make room if needed
    if ((err = _stream_clear(stream)))
        return err;

    LOG_DEBUG("%zu / %zu + %zu", len, stream_writebuf_size(stream), stream_readbuf_size(stream));

    // fill 'er up
    while (!len || stream_writebuf_size(stream) < len) {
        // needs moar bytez in mah buffers
        if (!stream_readbuf_size(stream)) {
            LOG_DEBUG("buffer became full while reading %zu of %zu", stream_writebuf_size(stream), len);
            return -1;
        }

        if ((err = _stream_read(stream)) < 0)
            return err;

        // handle EOF
        if (!err) {
            LOG_DEBUG("%zu of %zu", stream_writebuf_size(stream), len);
            continue;

        } else if (!stream_writebuf_size(stream)) {
            LOG_DEBUG("eof at start");
            return 1;

        } else if (!len) {
            LOG_DEBUG("read-to-eof gave %zu bytes", stream_writebuf_size(stream));
            break;

        } else {
            LOG_DEBUG("eof when reading %zu of %zu", stream_writebuf_size(stream), len);
            return 1;
        }
    }

    // terminate with NUL
    if (len) {
        if (_stream_insert(stream, '\0', len)) {
            LOG_DEBUG("stream writebuf became full when insrting NUL");
            return -1;
        }
    } else {
        if (_stream_append(stream, '\0')) {
            LOG_DEBUG("stream writebuf became full when appending NUL");
            return -1;
        }
    }

    *strp = stream_writebuf_ptr(stream);

    LOG_DEBUG("%s", *strp);

    // consume
    stream_write_consume(stream);

    return 0;
}

int stream_read_file (struct stream *stream, int fd, size_t *sizep)
{
    int err;
    ssize_t ret;

    // read() more if buffer empty; we should not block on read() while we still have data to process
    if (!stream_writebuf_size(stream)) {
        // make room if needed
        if ((err = _stream_clear(stream)))
            return err;

        // read in anything available
        if ((err = _stream_read(stream)) < 0)
            return err;

        // detect eof
        if (err > 0 && !stream_writebuf_size(stream)) {
            LOG_DEBUG("eof");
            return 1;
        }
    }

    size_t size = stream_writebuf_size(stream);

    if (*sizep && *sizep < size)
        // limit
        size = *sizep;

    if ((ret = write(fd, stream_writebuf_ptr(stream), size)) < 0) {
        LOG_ERROR("write %d: %s", fd, strerror(errno));
        return -1;
    }

    if (!ret) {
        LOG_DEBUG("write: eof");
        return 1;
    }

    stream_write_mark(stream, ret);

    // update
    *sizep = ret;

    // TODO: cleanup

    return 0;
}

/*
 * Flush any pending writes.
 *
 * On success, the write buffer will be empty.
 */
int stream_flush (struct stream *stream)
{
    int err;

    LOG_DEBUG("stream=%p, writebuf=%zu", stream, stream_writebuf_size(stream));

    // empty write buffer
    while (stream_writebuf_size(stream) > 0) {
        if ((err = _stream_write(stream)))
            return err;
    }

    // drop consumed data from write buffer
    if ((err = _stream_clear(stream)) < 0)
        return err;

    return 0;
}

int stream_write (struct stream *stream, const char *buf, size_t size)
{
    int err;

    // our write buffer must be empty, since _stream_write_direct will bypass it
    if ((err = stream_flush(stream)))
        return err;

    return _stream_write_direct(stream, buf, size);
}

int stream_vprintf (struct stream *stream, const char *fmt, va_list args)
{
    int ret, err;

    LOG_DEBUG("stream=%p fmt=%s", stream, fmt);

    if ((ret = vsnprintf(stream_readbuf_ptr(stream), stream_readbuf_size(stream), fmt, args)) < 0) {
        LOG_ERROR("vsnprintf");
        return -1;
    }

    if (ret >= stream_readbuf_size(stream))
        // full
        return 1;

    LOG_DEBUG("stream=%p mark ret=%d (%*s)", stream, ret, ret, stream_readbuf_ptr(stream));

    stream_read_mark(stream, ret);

    // TODO: write buffering
    if ((err = stream_flush(stream)))
        return err;

    return 0;
}

int stream_printf (struct stream *stream, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = stream_vprintf(stream, fmt, args);
    va_end(args);

    return ret;
}

/*
 * Fallback for sendfile.
 */
int _stream_write_file (struct stream *stream, int fd, size_t *sizep)
{
    int err;
    ssize_t ret;

    if ((err = _stream_clear(stream)))
        return err;

    // read into readbuf
    size_t size = stream_readbuf_size(stream);

    if (*sizep && *sizep < size)
        // limit
        size = *sizep;

    if ((ret = read(fd, stream_readbuf_ptr(stream), size)) < 0) {
        LOG_ERROR("read: %s", strerror(errno));
        return -1;
    }

    if (!ret) {
        LOG_DEBUG("read: eof");
        return 1;
    }

    stream_read_mark(stream, ret);

    // update
    *sizep = ret;

    // write out
    if ((err = _stream_write(stream)))
        return err;

    return 0;
}

int stream_sendfile (struct stream *stream, int fd, size_t *sizep)
{
    int err;

    if (!stream->type->sendfile)
        // fallback
        return _stream_write_file(stream, fd, sizep);

    // our write buffer must be empty, since sendfile will bypass it
    if ((err = stream_flush(stream)))
        return err;

    if ((err = stream->type->sendfile(fd, sizep, stream->ctx)))
        return err;

    return 0;
}

void stream_destroy (struct stream *stream)
{
    free(stream->buf);
    free(stream);
}
