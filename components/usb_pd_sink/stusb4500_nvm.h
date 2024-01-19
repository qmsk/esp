#pragma once

#include <stdint.h>

#define STUSB4500_NVM_SECTOR_COUNT 5
#define STUSB4500_NVM_SECTOR_SIZE 8

typedef uint8_t stusb4500_nvm_sector_t[STUSB4500_NVM_SECTOR_SIZE];

struct stusb4500_nvm {
  stusb4500_nvm_sector_t sectors[STUSB4500_NVM_SECTOR_COUNT];
};
