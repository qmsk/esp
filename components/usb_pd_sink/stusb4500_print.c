#include "stusb4500.h"
#include "stusb4500_i2c.h"

#include <esp_err.h>

#include <logging.h>

static void stusb4500_print_status(struct stusb4500 *stusb4500, FILE *file)
{
  struct stusb4500_i2c_rev rev;
  struct stusb4500_i2c_status status;
  struct stusb4500_pe_fsm pe_fsm;
  struct stusb4500_device_id device_id;
  int err;

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_BCD_TYPEC_REV_LOW, &rev, sizeof(rev)))) {
    LOG_ERROR("stusb4500_i2c_read STUSB4500_BCD_TYPEC_REV_LOW");
  } else {
    fprintf(file, "\tbcd_rev: typec_low=%u.%u typec_high=%u.%u usbpd_low=%u.%u usbpd_high=%u.%u\n",
      rev.bcd_typec_rev_low.major, rev.bcd_typec_rev_low.minor,
      rev.bcd_typec_rev_high.major, rev.bcd_typec_rev_high.minor,
      rev.bcd_usbpd_rev_low.major, rev.bcd_usbpd_rev_low.minor,
      rev.bcd_usbpd_rev_high.major, rev.bcd_usbpd_rev_high.minor
    );
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_PORT_STATUS_0, &status, sizeof(status)))) {
    LOG_ERROR("stusb4500_i2c_read STUSB4500_PORT_STATUS_0");
  } else {
    fprintf(file, "\tport_status: attach_trans=%u attached_device=%u power_mode=%u data_mode=%u attach=%u\n",
      status.port_status_0.attach_trans,
      status.port_status_1.attached_device,
      status.port_status_1.power_mode,
      status.port_status_1.data_mode,
      status.port_status_1.attach
    );

    fprintf(file, "\ttypec_monitoring_status: vbus_low_status=%u vbus_high_status=%u vbus_valid_snk=%u vbus_vsafe0v=%u vbus_ready=%u\n",
      status.typec_monitoring_status_0.vbus_low_status,
      status.typec_monitoring_status_0.vbus_high_status,
      status.typec_monitoring_status_1.vbus_valid_snk,
      status.typec_monitoring_status_1.vbus_vsafe0v,
      status.typec_monitoring_status_1.vbus_ready
    );

    fprintf(file, "\tcc_status: cc1_state=%u cc2_state=%u connect_result=%u looking_4_connection=%u\n",
      status.cc_status.cc1_state,
      status.cc_status.cc2_state,
      status.cc_status.connect_result,
      status.cc_status.looking_4_connection
    );

    fprintf(file, "\tcc_hw_fault_status: vbus_disch_fault=%u vpu_valid=%u vpu_ovp_fault=%u\n",
      status.cc_hw_fault_status_1.vbus_disch_fault,
      status.cc_hw_fault_status_1.vpu_valid,
      status.cc_hw_fault_status_1.vpu_ovp_fault
    );

    fprintf(file, "\tpd_typec_status: pd_typec_hand_check=%u\n",
      status.pd_typec_status.pd_typec_hand_check
    );

    fprintf(file, "\ttypec_status: typec_fsm_state=%u reverse=%u\n",
      status.typec_status.typec_fsm_state,
      status.typec_status.reverse
    );

    fprintf(file, "\tprt_status: prl_hw_rst_received=%u prl_msg_received=%u prt_bist_received=%u\n",
      status.prt_status.prl_hw_rst_received,
      status.prt_status.prl_msg_received,
      status.prt_status.prt_bist_received
    );

  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_PE_FSM, &pe_fsm, sizeof(pe_fsm)))) {
    LOG_ERROR("stusb4500_i2c_read STUSB4500_PE_FSM");
  } else {
    fprintf(file, "\tpe_fsm: pe_fsm_state=%u\n",
      pe_fsm.pe_fsm_state
    );
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_DEVICE_ID, &device_id, sizeof(device_id)))) {
    LOG_ERROR("stusb4500_i2c_read STUSB4500_DEVICE_ID");
  } else {
    fprintf(file, "\tdevice_id: device_id=%u\n",
      device_id.device_id
    );
  }
}

static void stusb4500_print_ctrl(struct stusb4500 *stusb4500, FILE *file)
{
   struct stusb4500_monitoring_ctrl_0 monitoring_ctrl_0;
   struct stusb4500_monitoring_ctrl_1 monitoring_ctrl_1;
   struct stusb4500_monitoring_ctrl_2 monitoring_ctrl_2;
   struct stusb4500_vbus_discharge_time_ctrl vbus_discharge_time_ctrl;
   int err;

   if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_MONITORING_CTRL_0, &monitoring_ctrl_0, sizeof(monitoring_ctrl_0)))) {
     LOG_ERROR("stusb4500_i2c_read STUSB4500_MONITORING_CTRL_0");
   } else if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_MONITORING_CTRL_1, &monitoring_ctrl_1, sizeof(monitoring_ctrl_1)))) {
     LOG_ERROR("stusb4500_i2c_read STUSB4500_MONITORING_CTRL_1");
   } else if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_MONITORING_CTRL_2, &monitoring_ctrl_2, sizeof(monitoring_ctrl_2)))) {
     LOG_ERROR("stusb4500_i2c_read STUSB4500_MONITORING_CTRL_2");
   } else {
     fprintf(file, "\tmonitoring_ctrl: vbus_snk_disc_threshold=%u voltage=%u vshift_low=%u vshift_high=%u\n",
       monitoring_ctrl_0.vbus_snk_disc_threshold,
       monitoring_ctrl_1.voltage,
       monitoring_ctrl_2.vshift_low, monitoring_ctrl_2.vshift_high
     );
   }

   if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_VBUS_DISCHARGE_TIME_CTRL, &vbus_discharge_time_ctrl, sizeof(vbus_discharge_time_ctrl)))) {
     LOG_ERROR("stusb4500_i2c_read STUSB4500_VBUS_DISCHARGE_TIME_CTRL");
   } else {
     fprintf(file, "\tvbus_discharge_time_ctrl: discharge_time_transition=%u discharge_time_to_0v=%u\n",
       vbus_discharge_time_ctrl.discharge_time_transition,
       vbus_discharge_time_ctrl.discharge_time_to_0v
     );
   }
}

static void stusb4500_print_pdo(struct stusb4500 *stusb4500, FILE *file)
{
  struct stusb4500_dpm_pdo_numb pdo_numb;
  union stusb4500_pdo pdo;
  int err;

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_DPM_PDO_NUMB, &pdo_numb, sizeof(pdo_numb)))) {
    LOG_ERROR("stusb4500_i2c_read STUSB4500_DPM_PDO_NUMB");
    return;
  }

  for (int i = 0; i < pdo_numb.dpm_snk_pdo_numb && i < STUSB4500_DPM_SNK_PDO_COUNT; i++) {
    if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_DPM_SNK_PDO(i), &pdo, sizeof(pdo)))) {
      LOG_ERROR("stusb4500_i2c_read STUSB4500_DPM_SNK_PDO(%d)", i);
      continue;
    }

    switch(pdo.header.type) {
      case STUSB4500_PDO_TYPE_FIXED_SUPPLY:
        fprintf(file, "\tdpm_snk_pdo%d: fixed_supply max_current=%u voltage=%u fast_role_swap=%u dual_role_data=%u usb_comm_capable=%u unconstrained_power=%u higher_capability=%u dual_role_power=%u\n", i,
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
}

static void stusb4500_print_rdo(struct stusb4500 *stusb4500, FILE *file)
{
  union stusb4500_rdo_reg_status rdo;
  int err;

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_RDO_REG_STATUS_0, &rdo, sizeof(rdo)))) {
    LOG_ERROR("stusb4500_i2c_read STUSB4500_RDO_REG_STATUS_0");
  } else {
    fprintf(file, "\trdo: fixed_supply max_current=%u operating_current=%u unchunked_messages_supported=%u no_usb_suspend=%u usb_comm_capable=%u capability_mismatch=%u give_back=%u object_position=%u\n",
      rdo.fixed_supply.max_current,
      rdo.fixed_supply.operating_current,
      rdo.fixed_supply.unchunked_messages_supported,
      rdo.fixed_supply.no_usb_suspend,
      rdo.fixed_supply.usb_comm_capable,
      rdo.fixed_supply.capability_mismatch,
      rdo.fixed_supply.give_back,
      rdo.fixed_supply.object_position
    );
  }
}

static void stusb4500_print_nvm(struct stusb4500 *stusb4500, FILE *file)
{
  union stusb4500_nvm nvm = {};
  int err;

  if ((err = stusb4500_nvm_read(stusb4500, &nvm))) {
    LOG_ERROR("stusb4500_nvm_read");
    return;
  }

  for (int i = 0; i < STUSB4500_NVM_SECTOR_COUNT; i++) {
    fprintf(file, "\tnvm sector[%d]: %02x %02x %02x %02x %02x %02x %02x %02x\n", i,
      nvm.sectors[i][0],
      nvm.sectors[i][1],
      nvm.sectors[i][2],
      nvm.sectors[i][3],
      nvm.sectors[i][4],
      nvm.sectors[i][5],
      nvm.sectors[i][6],
      nvm.sectors[i][7]
    );
  }

  fprintf(file, "\tnvm bank0 vendor_id=%04x product_id=%04x bcd_device_id=%04x port_role_ctrl=%u device_power_role_ctrl=%u\n",
    nvm.banks.bank0.vendor_id,
    nvm.banks.bank0.product_id,
    nvm.banks.bank0.bcd_device_id,
    nvm.banks.bank0.port_role_ctrl,
    nvm.banks.bank0.device_power_role_ctrl
  );

  fprintf(file, "\tnvm bank1 gpio_cfg=%u vbus_dchg_mask=%u vbus_disch_time_to_pdo=%u discharge_time_to_0v=%u\n",
    nvm.banks.bank1.gpio_cfg,
    nvm.banks.bank1.vbus_dchg_mask,
    nvm.banks.bank1.vbus_disch_time_to_pdo,
    nvm.banks.bank1.discharge_time_to_0v
  );

  fprintf(file, "\tnvm bank3 usb_comm_capable=%u dpm_snk_pdo_numb=%u snk_uncons_power=%u\n",
    nvm.banks.bank3.usb_comm_capable,
    nvm.banks.bank3.dpm_snk_pdo_numb,
    nvm.banks.bank3.snk_uncons_power
  );

  fprintf(file, "\tnvm bank3 pdo1(i=%u ll=%u hl=%u) pdo2(i=%u ll=%u hl=%u) pdo3(i=%u ll=%u hl=%u)\n",
    nvm.banks.bank3.lut_snk_pdo1_i,
    nvm.banks.bank3.snk_ll1,
    nvm.banks.bank3.snk_hl1,
    nvm.banks.bank3.lut_snk_pdo2_i,
    nvm.banks.bank3.snk_ll2,
    nvm.banks.bank3.snk_hl2,
    nvm.banks.bank3.lut_snk_pdo3_i,
    nvm.banks.bank3.snk_ll3,
    nvm.banks.bank3.snk_hl3
  );

  fprintf(file, "\tnvm bank4 snk_pdo_flex1_v=%u snk_pdo_flex2_v=%u snk_pdo_flex_i=%u power_ok_cfg=%u power_only_above_5v=%u req_src_current=%u\n",
    nvm.banks.bank4.snk_pdo_flex1_v,
    nvm.banks.bank4.snk_pdo_flex2_v,
    nvm.banks.bank4.snk_pdo_flex_i,
    nvm.banks.bank4.power_ok_cfg,
    nvm.banks.bank4.power_only_above_5v,
    nvm.banks.bank4.req_src_current
  );
}

int stusb4500_print(struct stusb4500 *stusb4500, FILE *file)
{
  stusb4500_print_status(stusb4500, file);
  stusb4500_print_ctrl(stusb4500, file);
  stusb4500_print_pdo(stusb4500, file);
  stusb4500_print_rdo(stusb4500, file);
  stusb4500_print_nvm(stusb4500, file);

  return 0;
}
