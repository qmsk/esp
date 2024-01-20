#include "stusb4500.h"
#include "stusb4500_nvm.h"

#include <sdkconfig.h>

#include <logging.h>

#include <string.h>

#ifndef CONFIG_STUSB4500_VBUS_DISCH_DISABLE
  #define CONFIG_STUSB4500_VBUS_DISCH_DISABLE 0
#endif

#ifndef CONFIG_STUSB4500_USB_COMM_CAPABLE
  #define CONFIG_STUSB4500_USB_COMM_CAPABLE 0
#endif

#ifndef CONFIG_STUSB4500_SNK_UNCONS_POWER
  #define CONFIG_STUSB4500_SNK_UNCONS_POWER 0
#endif

#ifndef CONFIG_STUSB4500_REQ_SRC_CURRENT
  #define CONFIG_STUSB4500_REQ_SRC_CURRENT 0
#endif

#ifndef CONFIG_STUSB4500_POWER_ONLY_ABOVE_5V
  #define CONFIG_STUSB4500_POWER_ONLY_ABOVE_5V 0
#endif

// convert from kconfig mV -> flex_v [50mV]
static inline unsigned config_snk_pdo_flex_v(unsigned config_mV)
{
  return config_mV / 50;
}

// convert from kconfig mA -> lut_i
static inline unsigned config_snk_pdo_i(unsigned config_mA)
{
  if (config_mA < 500) {
    return 0b001; // 0.5A
  } else if (config_mA < 3000) {
    return (config_mA - 500) / 250 + 1;
  } else if (config_mA < 5000) {
    return (config_mA - 3000) / 500 + 11;
  } else {
    return 0b1111;
  }
}

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

  if (nvm->banks.bank3.dpm_snk_pdo_numb != CONFIG_STUSB4500_SNK_PDO_NUMB) {
    nvm->banks.bank3.dpm_snk_pdo_numb = CONFIG_STUSB4500_SNK_PDO_NUMB;

    LOG_WARN("set CONFIG_STUSB4500_SNK_PDO_NUMB = %u", CONFIG_STUSB4500_SNK_PDO_NUMB);

    ret++;
  }

  if (nvm->banks.bank4.snk_pdo_flex1_v != config_snk_pdo_flex_v(CONFIG_STUSB4500_V_SNK_PDO2)) {
    nvm->banks.bank4.snk_pdo_flex1_v = config_snk_pdo_flex_v(CONFIG_STUSB4500_V_SNK_PDO2);

    LOG_WARN("set CONFIG_STUSB4500_V_SNK_PDO2 = %u", config_snk_pdo_flex_v(CONFIG_STUSB4500_V_SNK_PDO2));

    ret++;
  }

  if (nvm->banks.bank4.snk_pdo_flex2_v != config_snk_pdo_flex_v(CONFIG_STUSB4500_V_SNK_PDO3)) {
    nvm->banks.bank4.snk_pdo_flex2_v = config_snk_pdo_flex_v(CONFIG_STUSB4500_V_SNK_PDO3);

    LOG_WARN("set CONFIG_STUSB4500_V_SNK_PDO3 = %u", config_snk_pdo_flex_v(CONFIG_STUSB4500_V_SNK_PDO3));

    ret++;
  }

  if (nvm->banks.bank3.lut_snk_pdo1_i != config_snk_pdo_i(CONFIG_STUSB4500_I_SNK_PDO1)) {
    nvm->banks.bank3.lut_snk_pdo1_i = config_snk_pdo_i(CONFIG_STUSB4500_I_SNK_PDO1);

    LOG_WARN("set CONFIG_STUSB4500_I_SNK_PDO1 = %u", config_snk_pdo_i(CONFIG_STUSB4500_I_SNK_PDO1));

    ret++;
  }

  if (nvm->banks.bank3.lut_snk_pdo2_i != config_snk_pdo_i(CONFIG_STUSB4500_I_SNK_PDO2)) {
    nvm->banks.bank3.lut_snk_pdo2_i = config_snk_pdo_i(CONFIG_STUSB4500_I_SNK_PDO2);

    LOG_WARN("set STUSB4500_I_SNK_PDO2 = %u", config_snk_pdo_i(CONFIG_STUSB4500_I_SNK_PDO2));

    ret++;
  }

  if (nvm->banks.bank3.lut_snk_pdo3_i != config_snk_pdo_i(CONFIG_STUSB4500_I_SNK_PDO3)) {
    nvm->banks.bank3.lut_snk_pdo3_i = config_snk_pdo_i(CONFIG_STUSB4500_I_SNK_PDO3);

    LOG_WARN("set STUSB4500_I_SNK_PDO3 = %u", config_snk_pdo_i(CONFIG_STUSB4500_I_SNK_PDO3));

    ret++;
  }

  if (nvm->banks.bank3.dpm_snk_pdo_numb != CONFIG_STUSB4500_SNK_PDO_NUMB) {
    nvm->banks.bank3.dpm_snk_pdo_numb = CONFIG_STUSB4500_SNK_PDO_NUMB;

    LOG_WARN("set CONFIG_STUSB4500_SNK_PDO_NUMB = %u", CONFIG_STUSB4500_SNK_PDO_NUMB);

    ret++;
  }

  if (nvm->banks.bank1.vbus_dchg_mask != CONFIG_STUSB4500_VBUS_DISCH_DISABLE) {
    nvm->banks.bank1.vbus_dchg_mask = CONFIG_STUSB4500_VBUS_DISCH_DISABLE;

    LOG_WARN("set CONFIG_STUSB4500_VBUS_DISCH_DISABLE = %u", CONFIG_STUSB4500_VBUS_DISCH_DISABLE);

    ret++;
  }

  if (nvm->banks.bank3.usb_comm_capable != CONFIG_STUSB4500_USB_COMM_CAPABLE) {
    nvm->banks.bank3.usb_comm_capable = CONFIG_STUSB4500_USB_COMM_CAPABLE;

    LOG_WARN("set CONFIG_STUSB4500_USB_COMM_CAPABLE = %u", CONFIG_STUSB4500_USB_COMM_CAPABLE);

    ret++;
  }

  if (nvm->banks.bank3.snk_uncons_power != CONFIG_STUSB4500_SNK_UNCONS_POWER) {
    nvm->banks.bank3.snk_uncons_power = CONFIG_STUSB4500_SNK_UNCONS_POWER;

    LOG_WARN("set CONFIG_STUSB4500_SNK_UNCONS_POWER = %u", CONFIG_STUSB4500_SNK_UNCONS_POWER);

    ret++;
  }

  if (nvm->banks.bank4.req_src_current != CONFIG_STUSB4500_REQ_SRC_CURRENT) {
    nvm->banks.bank4.req_src_current = CONFIG_STUSB4500_REQ_SRC_CURRENT;

    LOG_WARN("set CONFIG_STUSB4500_REQ_SRC_CURRENT = %u", CONFIG_STUSB4500_REQ_SRC_CURRENT);

    ret++;
  }

  if (nvm->banks.bank4.power_ok_cfg != CONFIG_STUSB4500_POWER_OK_CFG) {
    nvm->banks.bank4.power_ok_cfg = CONFIG_STUSB4500_POWER_OK_CFG;

    LOG_WARN("set CONFIG_STUSB4500_POWER_OK_CFG = %u", CONFIG_STUSB4500_POWER_OK_CFG);

    ret++;
  }

  if (nvm->banks.bank4.power_only_above_5v != CONFIG_STUSB4500_POWER_ONLY_ABOVE_5V) {
    nvm->banks.bank4.power_only_above_5v = CONFIG_STUSB4500_POWER_ONLY_ABOVE_5V;

    LOG_WARN("set POWER_ONLY_ABOVE_5V = %u", CONFIG_STUSB4500_POWER_ONLY_ABOVE_5V);

    ret++;
  }

  if (nvm->banks.bank1.gpio_cfg != CONFIG_STUSB4500_GPIO_CFG) {
    nvm->banks.bank1.gpio_cfg = CONFIG_STUSB4500_GPIO_CFG;

    LOG_WARN("set STUSB4500_GPIO_CFG = %u", CONFIG_STUSB4500_GPIO_CFG);

    ret++;
  }

  return ret;
}
