#include "stusb4500.h"
#include "stusb4500_nvm.h"

#include <sdkconfig.h>

#include <logging.h>

#include <string.h>

int stusb4500_config_nvm(union stusb4500_nvm *nvm)
{
  stusb4500_nvm_sector_t zero_sector = {};

  // sanity-check
  for (int i = 0; i < STUSB4500_NVM_SECTOR_COUNT; i++) {
    if (!memcmp(nvm->sectors[i], &zero_sector, sizeof(zero_sector))) {
      LOG_ERROR("sector%d is all-zeroes, corrupted read?", i);
      return -1;
    }
  }

  for (int i = 1; i < STUSB4500_NVM_SECTOR_COUNT; i++) {
    if (!memcmp(nvm->sectors[i], nvm->sectors[i - 1], sizeof(nvm->sectors[i]))) {
      LOG_ERROR("sector%d is identical to sector%d, corrupted read?", i, i - 1);
      return -1;
    }
  }

  if (nvm->banks.bank0.vendor_id != STUSB4500_NVM_VENDOR_ID) {
    LOG_ERROR("bank0 vendor_id=%04x product_id=%04x bcd_device_id=%04x unsupported vendor_id",
      nvm->banks.bank0.vendor_id,
      nvm->banks.bank0.product_id,
      nvm->banks.bank0.bcd_device_id
    );
    return -1;
  }

  if (nvm->banks.bank0.product_id != STUSB4500_NVM_PRODUCT_ID) {
    LOG_ERROR("bank0 vendor_id=%04x product_id=%04x bcd_device_id=%04x unsupported product_id",
      nvm->banks.bank0.vendor_id,
      nvm->banks.bank0.product_id,
      nvm->banks.bank0.bcd_device_id
    );
    return -1;
  }

  if (nvm->banks.bank0.bcd_device_id != STUSB4500_NVM_DEVICE_ID) {
    LOG_ERROR("bank0 vendor_id=%04x product_id=%04x bcd_device_id=%04x unsupported bcd_device_id",
      nvm->banks.bank0.vendor_id,
      nvm->banks.bank0.product_id,
      nvm->banks.bank0.bcd_device_id
    );
    return -1;
  }

  // apply config changes
  int ret = 0;

  if (nvm->banks.bank4.power_only_above_5v != CONFIG_STUSB4500_POWER_ONLY_ABOVE_5V) {
    nvm->banks.bank4.power_only_above_5v = CONFIG_STUSB4500_POWER_ONLY_ABOVE_5V;

    LOG_WARN("set POWER_ONLY_ABOVE_5V = %u", CONFIG_STUSB4500_POWER_ONLY_ABOVE_5V);

    ret++;
  }

  return ret;
}
