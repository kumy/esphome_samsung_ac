#include "reset_used_power_button.h"

#include "esphome/core/log.h"

namespace esphome {
namespace samsung_ac {

void ResetUsedPowerButton::set_samsung_climat_uart(SamsungClimateUart *samsung_climate_uart) {
    this->samsung_climate_uart_ = samsung_climate_uart;
}

void ResetUsedPowerButton::press_action() {
    ESP_LOGD(TAG, "'%s': Sending data...", this->get_name().c_str());
    this->samsung_climate_uart_->sendCmd(0x1404, {{0xe8, {0x01}}});
}

void ResetUsedPowerButton::dump_config() { LOG_BUTTON("", "Reset Used Power Button", this); }

}  // namespace samsung_ac
}  // namespace esphome