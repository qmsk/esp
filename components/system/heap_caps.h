#pragma once

#include <esp_heap_caps.h>

#include <stddef.h>

// internals from components/heap/port/esp8266/esp_heap_init.c
extern heap_region_t g_heap_region[];

// internals from components/heap/src/esp_heap_caps.c
extern size_t g_heap_region_num;

size_t heap_caps_get_total_size(uint32_t caps);
