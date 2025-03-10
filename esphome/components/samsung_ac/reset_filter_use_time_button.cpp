#include "reset_filter_use_time_button.h"

#include "esphome/core/log.h"

namespace esphome {
namespace samsung_ac {

void ResetFilterUseTimeButton::set_samsung_climat_uart(SamsungClimateUart *samsung_climate_uart) {
    this->samsung_climate_uart_ = samsung_climate_uart;
}

void ResetFilterUseTimeButton::press_action() {
    ESP_LOGD(TAG, "'%s': Sending data...", this->get_name().c_str());
    this->samsung_climate_uart_->sendCmd(0x1304, {{0x44, {0xf0}}});
}

void ResetFilterUseTimeButton::dump_config() { LOG_BUTTON("", "Reset Filter Use Time Button", this); }

}  // namespace samsung_ac
}  // namespace esphome