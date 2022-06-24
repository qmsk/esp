#include <gpio_out.h>

#include <esp8266/eagle_soc.h>
#include <esp8266/gpio_struct.h>

#define RTC_GPIO_BIT 0x1

static inline void gpio_pins_out(uint32_t pins, uint32_t levels)
{
  if (pins & GPIO_OUT_PINS_HOST) {
    GPIO.out_w1ts = (pins & GPIO_OUT_PINS_HOST & levels);
    GPIO.out_w1tc = (pins & GPIO_OUT_PINS_HOST & ~levels);
  }

  if (pins & GPIO_OUT_PINS_RTC) {
    if (levels & GPIO_OUT_PINS_RTC) {
      SET_PERI_REG_MASK(RTC_GPIO_OUT, RTC_GPIO_BIT);
    } else {
      CLEAR_PERI_REG_MASK(RTC_GPIO_OUT, RTC_GPIO_BIT);
    }
  }
}
