#pragma once

#include <stddef.h>

struct stdio_log;

/*
 * Use log for stderr output.
 */
int stdio_attach_stderr_log(struct stdio_log *log);

/*
 * Allocate a ringbuffer for logging writes.
 */
int stdio_log_new(struct stdio_log **logp, size_t buffer_size);
