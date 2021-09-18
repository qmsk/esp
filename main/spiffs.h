#pragma once

#include <cmd.h>
#include <stddef.h>

/*
 * @return 0 on success, 1 if no partition defined, <0 on error
 */
int init_spiffs_partition(const char *base_path, const char *partition_label, size_t max_files);

/*
 * Formats partition on mount errors.
 *
 * @return 0 on success, 1 if no partition defined, <0 on error
 */
int init_spiffs_partition_formatted(const char *base_path, const char *partition_label, size_t max_files);

/*
 * Format partition
 */
int format_spiffs_partition(const char *partition_label);

extern const struct cmdtab spiffs_cmdtab;
