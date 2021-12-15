#pragma once

#include <esp_err.h>

/**
 * Register esp vfs for stdio 0/1/2 FDs.
 */
esp_err_t stdio_vfs_register();
