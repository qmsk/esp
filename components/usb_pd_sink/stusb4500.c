#include "stusb4500.h"
#include "stusb4500_i2c.h"

#include <esp_err.h>

#define DEBUG

#include <logging.h>

int stusb4500_init(struct stusb4500 *stusb4500, const struct usb_pd_sink_options *options)
{
  stusb4500->i2c_port = options->i2c_port;
  stusb4500->i2c_addr = USB_PD_SINK_STUSB4500_I2C_ADDR(options->i2c_addr);
  stusb4500->i2c_timeout = USB_PD_SINK_STUSB4500_I2C_TIMEOUT;

  return 0;
}

int stusb4500_get_status(struct stusb4500 *stusb4500)
{
  struct stusb4500_i2c_rev rev;
  struct stusb4500_i2c_status status;
  struct stusb4500_pe_fsm pe_fsm;
  struct stusb4500_device_id device_id;
  int err;

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_BCD_TYPEC_REV_LOW, &rev, sizeof(rev)))) {
    return err;
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_PORT_STATUS_0, &status, sizeof(status)))) {
    return err;
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_PE_FSM, &pe_fsm, sizeof(pe_fsm)))) {
    return err;
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_DEVICE_ID, &device_id, sizeof(device_id)))) {
    return err;
  }

  LOG_DEBUG("bcd_rev: typec_low=%u.%u typec_high=%u.%u usbpd_low=%u.%u usbpd_high=%u.%u",
    rev.bcd_typec_rev_low.major, rev.bcd_typec_rev_low.minor,
    rev.bcd_typec_rev_high.major, rev.bcd_typec_rev_high.minor,
    rev.bcd_usbpd_rev_low.major, rev.bcd_usbpd_rev_low.minor,
    rev.bcd_usbpd_rev_high.major, rev.bcd_usbpd_rev_high.minor
  );

  LOG_DEBUG("port_status: attach_trans=%u attached_device=%u power_mode=%u data_mode=%u attach=%u",
    status.port_status_0.attach_trans,
    status.port_status_1.attached_device,
    status.port_status_1.power_mode,
    status.port_status_1.data_mode,
    status.port_status_1.attach
  );

  LOG_DEBUG("typec_monitoring_status: vbus_low_status=%u vbus_high_status=%u vbus_valid_snk=%u vbus_vsafe0v=%u vbus_ready=%u",
    status.typec_monitoring_status_0.vbus_low_status,
    status.typec_monitoring_status_0.vbus_high_status,
    status.typec_monitoring_status_1.vbus_valid_snk,
    status.typec_monitoring_status_1.vbus_vsafe0v,
    status.typec_monitoring_status_1.vbus_ready
  );

  LOG_DEBUG("cc_status: cc1_state=%u cc2_state=%u connect_result=%u looking_4_connection=%u",
    status.cc_status.cc1_state,
    status.cc_status.cc2_state,
    status.cc_status.connect_result,
    status.cc_status.looking_4_connection
  );

  LOG_DEBUG("cc_hw_fault_status: vbus_disch_fault=%u vpu_valid=%u vpu_ovp_fault=%u",
    status.cc_hw_fault_status_1.vbus_disch_fault,
    status.cc_hw_fault_status_1.vpu_valid,
    status.cc_hw_fault_status_1.vpu_ovp_fault
  );

  LOG_DEBUG("pd_typec_status: pd_typec_hand_check=%u",
    status.pd_typec_status.pd_typec_hand_check
  );

  LOG_DEBUG("typec_status: typec_fsm_state=%u reverse=%u",
    status.typec_status.typec_fsm_state,
    status.typec_status.reverse
  );

  LOG_DEBUG("prt_status: prl_hw_rst_received=%u prl_msg_received=%u prt_bist_received=%u",
    status.prt_status.prl_hw_rst_received,
    status.prt_status.prl_msg_received,
    status.prt_status.prt_bist_received
  );

  LOG_DEBUG("pe_fsm: pe_fsm_state=%u",
    pe_fsm.pe_fsm_state
  );

  LOG_DEBUG("device_id: device_id=%u",
    device_id.device_id
  );

  return 0;
}

int stusb4500_get_ctrl(struct stusb4500 *stusb4500)
{
   struct stusb4500_monitoring_ctrl_0 monitoring_ctrl_0;
   struct stusb4500_monitoring_ctrl_1 monitoring_ctrl_1;
   struct stusb4500_monitoring_ctrl_2 monitoring_ctrl_2;
   struct stusb4500_vbus_discharge_time_ctrl vbus_discharge_time_ctrl;
   int err;

   if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_MONITORING_CTRL_0, &monitoring_ctrl_0, sizeof(monitoring_ctrl_0)))) {
     return err;
   }

   if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_MONITORING_CTRL_1, &monitoring_ctrl_1, sizeof(monitoring_ctrl_1)))) {
     return err;
   }

   if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_MONITORING_CTRL_2, &monitoring_ctrl_2, sizeof(monitoring_ctrl_2)))) {
     return err;
   }

   if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_VBUS_DISCHARGE_TIME_CTRL, &vbus_discharge_time_ctrl, sizeof(vbus_discharge_time_ctrl)))) {
     return err;
   }

   LOG_DEBUG("monitoring_ctrl: vbus_snk_disc_threshold=%u voltage=%u vshift_low=%u vshift_high=%u",
     monitoring_ctrl_0.vbus_snk_disc_threshold,
     monitoring_ctrl_1.voltage,
     monitoring_ctrl_2.vshift_low, monitoring_ctrl_2.vshift_high
   );

   return 0;
}

int stusb4500_get_pdo(struct stusb4500 *stusb4500)
{
  struct stusb4500_dpm_pdo_numb pdo_numb;
  union stusb4500_pdo pdo;
  int err;

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_DPM_PDO_NUMB, &pdo_numb, sizeof(pdo_numb)))) {
    return err;
  }

  for (int i = 0; i < pdo_numb.dpm_snk_pdo_numb && i < STUSB4500_DPM_SNK_PDO_COUNT; i++) {
    if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_DPM_SNK_PDO(i), &pdo, sizeof(pdo)))) {
      LOG_ERROR("stusb4500_i2c_read STUSB4500_DPM_SNK_PDO(%d)", i);
      return err;
    }

    switch(pdo.header.type) {
      case STUSB4500_PDO_TYPE_FIXED_SUPPLY:
        LOG_DEBUG("dpm_snk_pdo%d: fixed_supply max_current=%u voltage=%u fast_role_swap=%u dual_role_data=%u usb_comm_capable=%u unconstrained_power=%u higher_capability=%u dual_role_power=%u", i,
          pdo.fixed_supply.max_current,
          pdo.fixed_supply.voltage,
          pdo.fixed_supply.fast_role_swap,
          pdo.fixed_supply.dual_role_data,
          pdo.fixed_supply.usb_comm_capable,
          pdo.fixed_supply.unconstrained_power,
          pdo.fixed_supply.higher_capability,
          pdo.fixed_supply.dual_role_power
        );
        break;

      default:
        LOG_WARN("dpm_snk_pdo%d: unsupported type=%u", i, pdo.header.type);
        break;
    }
  }

  return 0;
}

int stusb4500_get_rdo(struct stusb4500 *stusb4500)
{
  union stusb4500_rdo_reg_status rdo;
  int err;

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_RDO_REG_STATUS_0, &rdo, sizeof(rdo)))) {
    return err;
  }

  LOG_DEBUG("rdo: fixed_supply max_current=%u operating_current=%u unchunked_messages_supported=%u no_usb_suspend=%u usb_comm_capable=%u capability_mismatch=%u give_back=%u object_position=%u",
    rdo.fixed_supply.max_current,
    rdo.fixed_supply.operating_current,
    rdo.fixed_supply.unchunked_messages_supported,
    rdo.fixed_supply.no_usb_suspend,
    rdo.fixed_supply.usb_comm_capable,
    rdo.fixed_supply.capability_mismatch,
    rdo.fixed_supply.give_back,
    rdo.fixed_supply.object_position
  );

  return 0;
}

static void stusb4500_nvm_debug(const union stusb4500_nvm *nvm)
{
  for (int i = 0; i < STUSB4500_NVM_SECTOR_COUNT; i++) {
    LOG_DEBUG("nvm sector[%d]: %02x %02x %02x %02x %02x %02x %02x %02x", i,
      nvm->sectors[i][0],
      nvm->sectors[i][1],
      nvm->sectors[i][2],
      nvm->sectors[i][3],
      nvm->sectors[i][4],
      nvm->sectors[i][5],
      nvm->sectors[i][6],
      nvm->sectors[i][7]
    );
  }

  LOG_DEBUG("nvm bank0 vendor_id=%04x product_id=%04x bcd_device_id=%04x port_role_ctrl=%u device_power_role_ctrl=%u",
    nvm->banks.bank0.vendor_id,
    nvm->banks.bank0.product_id,
    nvm->banks.bank0.bcd_device_id,
    nvm->banks.bank0.port_role_ctrl,
    nvm->banks.bank0.device_power_role_ctrl
  );

  LOG_DEBUG("nvm bank1 gpio_cfg=%u vbus_dchg_mask=%u vbus_disch_time_to_pdo=%u discharge_time_to_0v=%u",
    nvm->banks.bank1.gpio_cfg,
    nvm->banks.bank1.vbus_dchg_mask,
    nvm->banks.bank1.vbus_disch_time_to_pdo,
    nvm->banks.bank1.discharge_time_to_0v
  );

  LOG_DEBUG("nvm bank3 usb_comm_capable=%u dpm_snk_pdo_numb=%u snk_uncons_power=%u",
    nvm->banks.bank3.usb_comm_capable,
    nvm->banks.bank3.dpm_snk_pdo_numb,
    nvm->banks.bank3.snk_uncons_power
  );

  LOG_DEBUG("nvm bank3 pdo1(i=%u ll=%u hl=%u) pdo2(i=%u ll=%u hl=%u) pdo3(i=%u ll=%u hl=%u)",
    nvm->banks.bank3.lut_snk_pdo1_i,
    nvm->banks.bank3.snk_ll1,
    nvm->banks.bank3.snk_hl1,
    nvm->banks.bank3.lut_snk_pdo2_i,
    nvm->banks.bank3.snk_ll2,
    nvm->banks.bank3.snk_hl2,
    nvm->banks.bank3.lut_snk_pdo3_i,
    nvm->banks.bank3.snk_ll3,
    nvm->banks.bank3.snk_hl3
  );

  LOG_DEBUG("nvm bank4 snk_pdo_flex1_v=%u snk_pdo_flex2_v=%u snk_pdo_flex_i=%u power_ok_cfg=%u power_only_above_5v=%u req_src_current=%u",
    nvm->banks.bank4.snk_pdo_flex1_v,
    nvm->banks.bank4.snk_pdo_flex2_v,
    nvm->banks.bank4.snk_pdo_flex_i,
    nvm->banks.bank4.power_ok_cfg,
    nvm->banks.bank4.power_only_above_5v,
    nvm->banks.bank4.req_src_current
  );
}

int stusb4500_setup(struct stusb4500 *stusb4500)
{
  union stusb4500_nvm nvm = {};
  int err;

  if ((err = stusb4500_nvm_read(stusb4500, &nvm))) {
    LOG_ERROR("stusb4500_nvm_read");
    return err;
  }

  stusb4500_nvm_debug(&nvm);

  if ((err = stusb4500_config_nvm(&nvm)) < 0) {
    LOG_ERROR("stusb4500_config_nvm");
    return err;
  } else if (!err) {
    LOG_DEBUG("NVM OK");
    return 0;
  } else {
    LOG_WARN("NVM changes, write...");
  }

  if ((err = stusb4500_nvm_write(stusb4500, &nvm))) {
    LOG_ERROR("stusb4500_nvm_write");
    return err;
  }

  return 0;
}

int stusb4500_start(struct stusb4500 *stusb4500)
{
  int err;

  if ((err = stusb4500_get_status(stusb4500))) {
    LOG_ERROR("stusb4500_get_status");
    return err;
  }

  if ((err = stusb4500_get_ctrl(stusb4500))) {
    LOG_ERROR("stusb4500_get_ctrl");
    return err;
  }

  if ((err = stusb4500_get_pdo(stusb4500))) {
    LOG_ERROR("stusb4500_get_pdo");
    return err;
  }

  if ((err = stusb4500_get_rdo(stusb4500))) {
    LOG_ERROR("stusb4500_get_rdo");
    return err;
  }

  return 0;
}
