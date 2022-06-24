#include <gpio.h>

#include <esp8266/eagle_soc.h>
#include <esp8266/gpio_struct.h>

#define RTC_GPIO_BIT 0x1

static inline void gpio_host_out_pins(gpio_pins_t pins, gpio_pins_t levels)
{
  if (pins & GPIO_HOST_PINS_HOST) {
    GPIO.out_w1ts = (pins & GPIO_HOST_PINS_HOST & levels);
    GPIO.out_w1tc = (pins & GPIO_HOST_PINS_HOST & ~levels);
  }

  if (pins & GPIO_HOST_PINS_RTC) {
    if (levels & GPIO_HOST_PINS_RTC) {
      SET_PERI_REG_MASK(RTC_GPIO_OUT, RTC_GPIO_BIT);
    } else {
      CLEAR_PERI_REG_MASK(RTC_GPIO_OUT, RTC_GPIO_BIT);
    }
  }
}
