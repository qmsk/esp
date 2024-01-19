#pragma once

#define STUSB4500_DPM_SNK_PDO(i) (STUSB4500_DPM_SNK_PDO1_0 + 4 * (i))
#define STUSB4500_DPM_SNK_PDO_COUNT 3

enum stusb4500_i2c_register {
  STUSB4500_BCD_TYPEC_REV_LOW         = 0x06,
  STUSB4500_BCD_TYPEC_REV_HIGH        = 0x07,
  STUSB4500_BCD_USBPD_REV_LOW         = 0x08,
  STUSB4500_BCD_USBPD_REV_HIGH        = 0x09,
  STUSB4500_DEVICE_CAPAB_HIGH         = 0x0A,
  STUSB4500_ALERT_STATUS_1            = 0x0B,
  STUSB4500_ALERT_STATUS_1_MASK       = 0x0C,
  STUSB4500_PORT_STATUS_0             = 0x0D,
  STUSB4500_PORT_STATUS_1             = 0x0E,
  STUSB4500_TYPEC_MONITORING_STATUS_0 = 0x0F,
  STUSB4500_TYPEC_MONITORING_STATUS_1 = 0x10,
  STUSB4500_CC_STATUS                 = 0x11,
  STUSB4500_CC_HW_FAULT_STATUS_0      = 0x12,
  STUSB4500_CC_HW_FAULT_STATUS_1      = 0x13,
  STUSB4500_PD_TYPEC_STATUS           = 0x14,
  STUSB4500_TYPEC_STATUS              = 0x15,
  STUSB4500_PRT_STATUS                = 0x16,

  STUSB4500_PE_FSM                    = 0x29,

  STUSB4500_DEVICE_ID                 = 0x2F,

  STUSB4500_DPM_PDO_NUMB              = 0x70,
  STUSB4500_DPM_SNK_PDO1_0            = 0x85,
  STUSB4500_DPM_SNK_PDO1_1            = 0x86,
  STUSB4500_DPM_SNK_PDO1_2            = 0x87,
  STUSB4500_DPM_SNK_PDO1_3            = 0x88,
  STUSB4500_DPM_SNK_PDO2_0            = 0x89,
  STUSB4500_DPM_SNK_PDO2_1            = 0x8A,
  STUSB4500_DPM_SNK_PDO2_2            = 0x8B,
  STUSB4500_DPM_SNK_PDO2_3            = 0x8C,
  STUSB4500_DPM_SNK_PDO3_0            = 0x8D,
  STUSB4500_DPM_SNK_PDO3_1            = 0x8E,
  STUSB4500_DPM_SNK_PDO3_2            = 0x8F,
  STUSB4500_DPM_SNK_PDO3_3            = 0x90,
  STUSB4500_RDO_REG_STATUS_0          = 0x91,
  STUSB4500_RDO_REG_STATUS_1          = 0x92,
  STUSB4500_RDO_REG_STATUS_2          = 0x93,
  STUSB4500_RDO_REG_STATUS_3          = 0x94,
};

enum stusb4500_port_status_attached_device {
  STUSB4500_PORT_STATUS_ATTACHED_DEVICE_NONE  = 0b000,
  STUSB4500_PORT_STATUS_ATTACHED_DEVICE_SINK  = 0b001,
  STUSB4500_PORT_STATUS_ATTACHED_DEVICE_DEBUG = 0b011,
};

enum stusb4500_typec_fsm_state {
  STUSB4500_TYPEC_FSM_STATE_UNATTACHED_SNK        = 0b00000,
  STUSB4500_TYPEC_FSM_STATE_ATTACHWAIT_SNK        = 0b00001,
  STUSB4500_TYPEC_FSM_STATE_ATTACHED_SNK          = 0b00010,
  STUSB4500_TYPEC_FSM_STATE_DEBUGACCESSORY_SNK    = 0b00011,
  STUSB4500_TYPEC_FSM_STATE_TRY_SRC               = 0b01100,
  STUSB4500_TYPEC_FSM_STATE_UNATTACHED_ACCESSORY  = 0b01101,
  STUSB4500_TYPEC_FSM_STATE_ATTACHWAIT_ACCESSORY  = 0b01110,
  STUSB4500_TYPEC_FSM_STATE_TYPEC_ERRORRECOVERY   = 0b10011,
};

enum stusb4500_pdo_type {
  STUSB4500_PDO_TYPE_FIXED_SUPPLY     = 0b00,
  STUSB4500_PDO_TYPE_BATTERY          = 0b01,
  STUSB4500_PDO_TYPE_VARIABLE_SUPPLY  = 0b10,
  // Augmented Power Data Object (APDO) = 0b11, // XXX: PD 3.0
};

struct stusb4500_i2c_rev {
  // STUSB4500_BCD_TYPEC_REV_LOW
  struct stusb4500_bcd_typec_rev_low {
    uint8_t minor : 4;
    uint8_t major : 4;
  } bcd_typec_rev_low;

  // STUSB4500_BCD_TYPEC_REV_HIGH
  struct stusb4500_bcd_typec_rev_high {
    uint8_t minor : 4;
    uint8_t major : 4;
  } bcd_typec_rev_high;

  // STUSB4500_BCD_USBPD_REV_LOW
  struct stusb4500_bcd_usbpd_rev_low {
    uint8_t minor : 4;
    uint8_t major : 4;
  } bcd_usbpd_rev_low;

  struct stusb4500_bcd_usbpd_rev_high {
    uint8_t minor : 4;
    uint8_t major : 4;
  } bcd_usbpd_rev_high;
};

struct stusb4500_device_capab_high {
  uint8_t not_used;
};

struct stusb4500_i2c_alert_status {
  struct stusb4500_alert_status_1 {
    uint8_t reserved0 : 1;
    uint8_t prt_status_al : 1;
    uint8_t reserved2 : 1;
    uint8_t pd_typec_status_al : 1;
    uint8_t cc_hw_fault_status_al : 1;
    uint8_t typec_monitoring_status_al : 1;
    uint8_t port_status_al : 1;
    uint8_t reserved7 : 1;
  } alert_status_1 ;

  struct stusb4500_alert_status_1_mask {
    uint8_t bits;
  } alert_status_1_mask;
};

struct stusb4500_i2c_status {
  struct stusb4500_port_status_0 {
    uint8_t attach_trans    : 1; // Transition detected in attached state
    uint8_t reserved1       : 7;
  } port_status_0;

  struct stusb4500_port_status_1 {
    uint8_t attach          : 1; // 1 = ATTACHED
    uint8_t reserved2       : 1;
    uint8_t data_mode       : 1; // 0 = UFP
    uint8_t power_mode      : 1; // 0 = device is sinking power
    uint8_t reserved3       : 1;
    uint8_t attached_device : 3; // STUSB4500_PORT_STATUS_ATTACHED_DEVICE_*
  } port_status_1;

  struct stusb4500_typec_monitoring_status_0 {
    uint8_t reserved0 : 1;
    uint8_t vbus_valid_snk_trans : 1;
    uint8_t vbus_vsafe0v_trans : 1;
    uint8_t vbus_ready_trans : 1;
    uint8_t vbus_low_status : 1;
    uint8_t vbus_high_status : 1;
    uint8_t reserved6 : 2;
  } typec_monitoring_status_0;

  struct stusb4500_typec_monitoring_status_1 {
    uint8_t reserved0 : 1;
    uint8_t vbus_valid_snk : 1;
    uint8_t vbus_vsafe0v : 1;
    uint8_t vbus_ready : 1;
    uint8_t reserved4 : 4;
  } typec_monitoring_status_1;

  struct stusb4500_cc_status {
    uint8_t cc1_state : 1;
    uint8_t cc2_state : 1;
    uint8_t connect_result : 1;
    uint8_t looking_4_connection : 1;
    uint8_t reserved4 : 4;
  } cc_status;

  struct stusb4500_cc_hw_fault_status_0 {
    uint8_t reserved0 : 4;
    uint8_t vpu_ovp_fault_trans : 1;
    uint8_t vpu_valid_trans : 1;
    uint8_t reserved6 : 2;
  } cc_hw_fault_status_0;

  struct stusb4500_cc_hw_fault_status_1 {
    uint8_t reserved0 : 4;
    uint8_t vbus_disch_fault : 1;
    uint8_t reserved5 : 1;
    uint8_t vpu_valid : 1;
    uint8_t vpu_ovp_fault : 1;
  } cc_hw_fault_status_1;

  struct stusb4500_pd_typec_status {
    uint8_t pd_typec_hand_check : 4;
    uint8_t reserved4 : 4;
  } pd_typec_status;

  struct stusb4500_typec_status {
    uint8_t typec_fsm_state : 5; // enum stusb4500_typec_fsm_state
    uint8_t reserved5 : 1;
    uint8_t reserved6 : 1;
    uint8_t reverse : 1;
  } typec_status;

  struct stusb4500_prt_status {
    uint8_t prl_hw_rst_received : 1;
    uint8_t reserved1 : 1;
    uint8_t prl_msg_received : 1;
    uint8_t reserved3 : 1;
    uint8_t prt_bist_received : 1;
    uint8_t reserved5 : 3;
  } prt_status;
};

struct stusb4500_pe_fsm {
  uint8_t pe_fsm_state : 8;
};

struct stusb4500_device_id {
  uint8_t device_id : 8;
};

struct stusb4500_dpm_pdo_numb {
  uint8_t dpm_snk_pdo_numb : 3;
  uint8_t reserved3 : 5;
};

union stusb4500_pdo {
  struct stusb4500_pdo_header {
    uint32_t body : 30;
    uint32_t type : 2;
  } header;

  struct stusb4500_sink_fixed_supply_pdo {
    uint32_t max_current : 10; // 10mA
    uint32_t voltage : 10; // 50mV
    uint32_t reserved20 : 3; // peak_current for source
    uint32_t fast_role_swap : 2; // XXX: PD 3.0
    uint32_t dual_role_data : 1;
    uint32_t usb_comm_capable : 1;
    uint32_t unconstrained_power : 1;
    uint32_t higher_capability : 1; // usb_suspend_supported for source
    uint32_t dual_role_power : 1;
    uint32_t type : 2;
  } fixed_supply;
};

union stusb4500_rdo_reg_status {
  struct stusb4500_fuxed_supply_rdo {
    uint32_t max_current : 10; // 10mA
    uint32_t operating_current : 10; // 10mA
    uint32_t reserved20 : 3;
    uint32_t unchunked_messages_supported : 1; // XXX: PD3.0
    uint32_t no_usb_suspend : 1;
    uint32_t usb_comm_capable : 1;
    uint32_t capability_mismatch : 1;
    uint32_t give_back : 1;
    uint32_t object_position : 3;
    uint32_t reserved31 : 1;
  } fixed_supply;
};
