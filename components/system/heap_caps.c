#include "heap_caps.h"

// adapted from components/heap/src/esp_heap_caps.c
size_t heap_caps_get_total_size(uint32_t caps)
{
    size_t bytes = 0;

    for (int i = 0; i < g_heap_region_num; i++)
        if (caps == (caps & g_heap_region[i].caps))
            bytes += g_heap_region[i].total_size;

    return bytes;
}
