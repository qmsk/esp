#pragma once

#include <atx_psu.h>

#define ATX_PSU_BIT_LEDS1   ATX_PSU_BIT_1
#define ATX_PSU_BIT_LEDS2   ATX_PSU_BIT_2
#define ATX_PSU_BIT_LEDS3   ATX_PSU_BIT_3
#define ATX_PSU_BIT_LEDS4   ATX_PSU_BIT_4

/* Set ATX power_enable */
void set_atx_psu_bit(enum atx_psu_bit bit);

/* Set ATX power_enable, wait power_good */
int wait_atx_psu_bit(enum atx_psu_bit bit, TickType_t timeout);

/* Clear ATX power_enable */
void clear_atx_psu_bit(enum atx_psu_bit bit);
