#pragma once

#include <config.h>

extern const struct configtab atx_psu_configtab[];

enum atx_psu_bit {
  ATX_PSU_BIT_CLEAR   = 0,
  ATX_PSU_BIT_SPI_LED,

  ATX_PSU_BIT_COUNT
};

int init_atx_psu();

void activate_atx_psu(enum atx_psu_bit bit);
void deactivate_atx_psu(enum atx_psu_bit bit);
