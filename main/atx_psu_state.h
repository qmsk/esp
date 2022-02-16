#pragma once

#include <atx_psu.h>

#define ATX_PSU_BIT_LEDS1   ATX_PSU_BIT_1
#define ATX_PSU_BIT_LEDS2   ATX_PSU_BIT_2
#define ATX_PSU_BIT_LEDS3   ATX_PSU_BIT_3
#define ATX_PSU_BIT_LEDS4   ATX_PSU_BIT_4

void activate_atx_psu(enum atx_psu_bit bit);
void deactivate_atx_psu(enum atx_psu_bit bit);
