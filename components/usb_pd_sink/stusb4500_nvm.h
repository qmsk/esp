#pragma once

#include <stdint.h>

#define STUSB4500_NVM_SECTOR_COUNT 5
#define STUSB4500_NVM_SECTOR_SIZE 8

typedef uint8_t stusb4500_nvm_sector_t[STUSB4500_NVM_SECTOR_SIZE];

union stusb4500_nvm {
  stusb4500_nvm_sector_t sectors[STUSB4500_NVM_SECTOR_COUNT];

  // from STUSB4500_NVM TNxxx
  // https://community.st.com/t5/interface-and-connectivity-ics/about-the-nvm-programming-for-stusb4500/m-p/378217/highlight/true#M6664
  struct stusb4500_banks {
    struct stusb4500_bank0 {
      uint16_t vendor_id; // 0x0000

      uint16_t product_id; // 0xAAB0

      uint16_t bcd_device_id; // 0x4500

      uint16_t port_role_ctrl : 8; // 0x00
      uint16_t device_power_role_ctrl : 8; // 0x00
    } bank0;

    struct stusb4500_bank1 {
      uint8_t reserved0_0 : 4; // 0x0
      uint8_t gpio_cfg : 2;
      uint8_t reserved0_6 : 2; // 0b00

      uint8_t reserved1_0 : 5; // 0b00000
      uint8_t vbus_dchg_mask : 1;
      uint8_t reserved1_6 : 1;  // 1
      uint8_t reserved1_7 : 1;  // 0

      uint8_t vbus_disch_time_to_pdo : 4;
      uint8_t discharge_time_to_0v : 4;

      uint8_t reserved3 : 8; // 0x1C
      uint8_t reserved4 : 8; // 0xFF
      uint8_t reserved5 : 8; // 0x01
      uint8_t reserved6 : 8; // 0x3C
      uint8_t reserved7 : 8; // 0xDF
    } bank1;

    struct stusb4500_bank2 {
      uint8_t reserved0 : 8; // 0x02
      uint8_t reserved1 : 8; // 0x40
      uint8_t reserved2 : 8; // 0x0F
      uint8_t reserved3 : 8; // 0x00
      uint8_t reserved4 : 8; // 0x32
      uint8_t reserved5 : 8; // 0x00
      uint8_t reserved6 : 8; // 0xFC
      uint8_t reserved7 : 8; // 0xF1
    } bank2;

    struct stusb4500_bank3 {
      uint8_t reserved0 : 8; // 0x00
      uint8_t reserved1 : 8; // 0x19

      uint8_t usb_comm_capable : 1;
      uint8_t dpm_snk_pdo_numb : 2;
      uint8_t snk_uncons_power : 1;
      uint8_t lut_snk_pdo1_i : 4;

      uint8_t snk_ll1 : 4;
      uint8_t snk_hl1 : 4;

      uint8_t lut_snk_pdo2_i : 4;
      uint8_t snk_ll2 : 4;

      uint8_t snk_hl2 : 4;
      uint8_t lut_snk_pdo3_i : 4;

      uint8_t snk_ll3 : 4;
      uint8_t snk_hl3 : 4;

      uint8_t reserved7 : 8; // SNK_PDO_FILL_0xDF = 0x00
    } bank3;

    struct stusb4500_bank4 {
      uint64_t reserved0_0 : 6; // 0b000000
      uint64_t snk_pdo_flex1_v : 10;
      uint64_t snk_pdo_flex2_v : 10;
      uint64_t snk_pdo_flex_i : 10;
      uint64_t reserved5_4 : 1;  // 0
      uint64_t power_ok_cfg : 2;
      uint64_t reserved5_7 : 1;  // 0
      uint64_t reserved6_0 : 4; // 0x0
      uint64_t req_src_current : 1;
      uint64_t reserved6_5 : 3; // 0b010
      uint64_t reserved7 : 8; // ALERT_STATUS_1_MASK
    } bank4;
  } banks;
};
