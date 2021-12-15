#pragma once

#include <sys/types.h>
#include <stddef.h>

/**
 * Low-level system I/O routines used as a fallback during boot.
 */

ssize_t ets_write(const char *data, size_t size);
