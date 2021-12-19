#pragma once

#include <esp8266/eagle_soc.h>
#include <esp8266/pin_mux_register.h>

// glitchless version of PIN_FUNC_SELECT(), does not reset pin func to default if already set
#define IDEMPOTENT_PIN_FUNC_SELECT(PIN_NAME, FUNC) \
  SET_PERI_REG_BITS(PIN_NAME, PERIPHS_IO_MUX_FUNC, ((FUNC & 0x4) << 2) | (FUNC & 0x3), PERIPHS_IO_MUX_FUNC_S)
