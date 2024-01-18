#include "stusb4500.h"
#include "stusb4500_i2c.h"

#include <driver/i2c.h>
#include <esp_err.h>

#define DEBUG

#include <logging.h>

int stusb4500_init(struct stusb4500 *stusb, const struct usb_pd_sink_options *options)
{
  stusb->i2c_port = options->i2c_port;
  stusb->i2c_addr = USB_PD_SINK_STUSB4500_I2C_ADDR(options->i2c_addr);
  stusb->i2c_timeout = USB_PD_SINK_STUSB4500_I2C_TIMEOUT;

  return 0;
}

static int stusb4500_read(struct stusb4500 *stusb, enum stusb4500_i2c_register reg, void *out, size_t size)
{
  uint8_t cmd[] = { reg };
  esp_err_t err;

  if ((err = i2c_master_write_read_device(stusb->i2c_port, stusb->i2c_addr, cmd, sizeof(cmd), out, size, stusb->i2c_timeout))) {
    LOG_ERROR("i2c_master_write_to_device port=%d addr=%u: %s", stusb->i2c_port, stusb->i2c_addr, esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int stusb4500_start(struct stusb4500 *stusb)
{
  struct stusb4500_i2c_rev rev;
  struct stusb4500_i2c_status status;
  int err;

  if ((err = stusb4500_read(stusb, STUSB4500_BCD_TYPEC_REV_LOW, &rev, sizeof(rev)))) {
    return err;
  }

  if ((err = stusb4500_read(stusb, STUSB4500_PORT_STATUS_0, &status, sizeof(status)))) {
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

  return 0;
}
