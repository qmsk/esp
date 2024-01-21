#include "stusb4500.h"
#include "stusb4500_i2c.h"

#include <esp_err.h>

#include <logging.h>

int stusb4500_init(struct stusb4500 *stusb4500, const struct usb_pd_sink_options *options)
{
  stusb4500->i2c_port = options->i2c_port;
  stusb4500->i2c_addr = USB_PD_SINK_STUSB4500_I2C_ADDR(options->i2c_addr);
  stusb4500->i2c_timeout = USB_PD_SINK_STUSB4500_I2C_TIMEOUT;

  return 0;
}

int stusb4500_reset(struct stusb4500 *stusb4500)
{
  struct stusb4500_reset_ctrl reset_ctrl = { .reset_sw_en = 1 };
  struct stusb4500_reset_ctrl reset_ctrl_clear = { .reset_sw_en = 0 };
  int err;

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_RESET_CTRL, &reset_ctrl, sizeof(reset_ctrl)))) {
    return err;
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_RESET_CTRL, &reset_ctrl, sizeof(reset_ctrl)))) {
    return err;
  }

  // TODO: delay 25ms?
  LOG_DEBUG("reset_ctrl: reset_sw_en=%u",
    reset_ctrl.reset_sw_en
  );

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_RESET_CTRL, &reset_ctrl_clear, sizeof(reset_ctrl_clear)))) {
    return err;
  }

  return 0;
}

int stusb4500_setup(struct stusb4500 *stusb4500)
{
  union stusb4500_nvm nvm = {};
  int err;

  if ((err = stusb4500_nvm_read(stusb4500, &nvm))) {
    LOG_ERROR("stusb4500_nvm_read");
    return err;
  }

  if ((err = stusb4500_config_nvm(&nvm)) < 0) {
    LOG_ERROR("stusb4500_config_nvm");
    return err;
  } else if (!err) {
    LOG_INFO("NVM OK");
  } else {
    LOG_WARN("NVM changes, write...");

    if ((err = stusb4500_nvm_write(stusb4500, &nvm))) {
      LOG_ERROR("stusb4500_nvm_write");
      return err;
    }
  }

  return 0;
}

int stusb4500_start(struct stusb4500 *stusb4500)
{
  return 0;
}

int stusb4500_status(struct stusb4500 *stusb4500, struct usb_pd_sink_status *status)
{
  struct stusb4500_bcd_usbpd_rev_high bcd_usbpd_rev_high;
  struct stusb4500_port_status_1 port_status_1;
  struct stusb4500_typec_monitoring_status_1 typec_monitoring_status_1;
  struct stusb4500_monitoring_ctrl_1 monitoring_ctrl_1;
  union stusb4500_rdo_reg_status rdo;
  int err;

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_BCD_USBPD_REV_HIGH, &bcd_usbpd_rev_high, sizeof(bcd_usbpd_rev_high)))) {
    return err;
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_PORT_STATUS_1, &port_status_1, sizeof(port_status_1)))) {
    return err;
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_TYPEC_MONITORING_STATUS_1, &typec_monitoring_status_1, sizeof(typec_monitoring_status_1)))) {
    return err;
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_MONITORING_CTRL_1, &monitoring_ctrl_1, sizeof(monitoring_ctrl_1)))) {
    return err;
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_RDO_REG_STATUS_0, &rdo, sizeof(rdo)))) {
    return err;
  }

  status->usb_pd_version_major = bcd_usbpd_rev_high.major;
  status->usb_pd_version_minor = bcd_usbpd_rev_high.minor;

  status->port_attached = (port_status_1.attach == 1);
  status->port_power = (port_status_1.power_mode == 0);
  status->vbus_ready = (typec_monitoring_status_1.vbus_ready == 1);

  status->pdo_number = rdo.fixed_supply.object_position;
  status->pd_capability_mismatch = rdo.fixed_supply.capability_mismatch;

  status->voltage_mV = monitoring_ctrl_1.voltage * 100; // 100mV -> mV
  status->operating_current_mA = rdo.fixed_supply.operating_current * 10; // 10mA -> mA
  status->maximum_current_mA = rdo.fixed_supply.max_current * 10; // 10mA -> mA

  return 0;
}
