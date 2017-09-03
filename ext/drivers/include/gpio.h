#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdbool.h>

enum GPIO {
  GPIO_0 = 0,
  GPIO_1,
  GPIO_2,
  GPIO_3,
  GPIO_4,
  GPIO_5,
  GPIO_6,
  GPIO_7,
  GPIO_8,
  GPIO_9,
  GPIO_10,
  GPIO_11,
  GPIO_12,
  GPIO_13,
  GPIO_14,
  GPIO_15,

  GPIO_COUNT,
};

enum GPIO_OutputMode {
  GPIO_OUTPUT               = 0,

  GPIO_OUTPUT_LOW           = 0, // initialize output low
  GPIO_OUTPUT_HIGH          = 1, // initialize output high

  GPIO_OUTPUT_OPEN_DRAIN    = 2, // TODO: low => floating, high => pull output to ground?
};

bool GPIO_Exists(enum GPIO gpio);

void GPIO_SetupOutput(enum GPIO gpio, enum GPIO_OutputMode mode);

void GPIO_OutputEnable(enum GPIO gpio);
void GPIO_Output(enum GPIO gpio, bool level);
void GPIO_OutputHigh(enum GPIO gpio);
void GPIO_OutputLow(enum GPIO gpio);

#endif
