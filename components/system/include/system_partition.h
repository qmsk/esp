#pragma once

#include <esp_partition.h>

/* esp_partition.c */
const char *esp_partition_type_str(esp_partition_type_t type);
const char *esp_partition_subtype_str(esp_partition_type_t type, esp_partition_subtype_t subtype);
