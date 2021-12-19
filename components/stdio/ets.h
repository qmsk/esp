#pragma once

#include <sys/types.h>
#include <stddef.h>

/**
 * Low-level system I/O routines used as a fallback during boot.
 */
void ets_write(const char *data, size_t size);
