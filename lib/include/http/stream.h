#ifndef STREAM_H
#define STREAM_H

#include <stdlib.h>
#include <stdarg.h>

/*
 * Blocking SOCK_STREAM interface.
 *
 * These functions should always make some progress, updating *sizep, when returning 0.
 *
 * EOF and timeout are handled similarly, with the difference that on EOF *sizep is 0, whereas on timeout
 * *sizep remains set to nonzero.
 *
 * Return <0 on error, 1 on EOF/timeout, 0 on success.
 */
struct stream_type {
    int (*read)(char *buf, size_t *sizep, void *ctx);
    int (*write)(const char *buf, size_t *sizep, void *ctx);
    int (*sendfile)(int fd, size_t *sizep, void *ctx);
};

struct stream {
    const struct stream_type *type;

    char *buf;

    // note that offset <= length <= size at all times
    /* The amount of leading data in the buffer that has already been consumed */
    size_t offset;

    /* The amount of valid data existing in the buffer */
    size_t length;

    /* The total length of the buffer */
    size_t size;

    void *ctx;
};

/*
 * Construct a new stream, using the given implementation.
 *
 * `size` determines the fixed buffer allocated for the stream, which determines the maximum read/write size.
 */
int stream_create (const struct stream_type *type, struct stream **streamp, size_t size, void *ctx);

/*
 * Read binary data from the stream.
 *
 * *sizep may be passed as 0 to read as much data as available, or a maximum amount to return.
 *
 * The amount of data available is returned in *sizep, and may be less on EOF.
 *
 * Returns 1 on EOF, <0 on error.
 */
int stream_read (struct stream *stream, char **bufp, size_t *sizep);

/*
 * Read one line from the stream, returning a pointer to the NUL-terminated line.
 *
 * Returns 1 on EOF, <0 on error.
 */
int stream_read_line (struct stream *stream, char **linep);

/*
 * Read stream as a string, returning a pointer to the NUL-terminated data.
 *
 * The maximum size to read should be given in len, or 0 to read to EOF.
 *
 * Returns 1 on EOF, <0 on error.
 */
int stream_read_string (struct stream *stream, char **strp, size_t len);

/*
 * Copy from stream into a FILE.
 *
 * *sizep is the number of bytes to read from stream, or zero to read until EOF.
 * *sizep is updated on return to reflect the amount of bytes copied, which may be less than *sizep.
 *
 * Returns <0 on error, 0 on success, >0 on EOF.
 */
int stream_read_file (struct stream *stream, int fd, size_t *sizep);

/*
 * Write out the full contents of the given buffer to the stream.
 */
int stream_write (struct stream *stream, const char *buf, size_t size);

/*
 * Write arbitrary formatted output to the stream.
 */
int stream_vprintf (struct stream *stream, const char *fmt, va_list args);
int stream_printf (struct stream *stream, const char *fmt, ...)
    __attribute((format (printf, 2, 3)));

/*
 * Copy to stream from a file, bypassing the buffer if the stream_type implements it.
 *
 * *sizep is the number of bytes to be sent from fd, or zero to send until EOF.
 * *sizep is updated on return to reflect the amount of bytes copied, which may be less then *sizep.
 *
 * Returns <0 on error, 0 on success, >0 on EOF.
 */
int stream_sendfile (struct stream *stream, int fd, size_t *sizep);

/*
 * Release all resources.
 */
void stream_destroy (struct stream *stream);

#endif
