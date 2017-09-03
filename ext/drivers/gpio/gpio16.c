#include "gpio.h"

#include <c_types.h>
#include <esp8266/eagle_soc.h>

#define WRITE_PERI_REG_BITS(reg, mask, bits) WRITE_PERI_REG((reg), ((READ_PERI_REG(reg) & (~(mask)))) | ((bits) & (mask)))

void GPIO16_OutputEnable()
{
  WRITE_PERI_REG_BITS(PAD_XPD_DCDC_CONF, 0x43, 0x1);
  WRITE_PERI_REG_BITS(RTC_GPIO_CONF, 0x1, 0x0);
  WRITE_PERI_REG_BITS(RTC_GPIO_ENABLE, 0x1, 0x1);
}

void GPIO16_Output(bool value)
{
  WRITE_PERI_REG(RTC_GPIO_OUT, value ? 0x1 : 0);
}
