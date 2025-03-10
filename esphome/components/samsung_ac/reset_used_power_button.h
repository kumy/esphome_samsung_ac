#pragma once

#include <vector>

#include "esphome/components/button/button.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "samsung_climate.h"

namespace esphome {
namespace samsung_ac {

class ResetUsedPowerButton : public button::Button, public Component {
   public:
    void set_samsung_climat_uart(SamsungClimateUart* samsung_climate_uart);

    void dump_config() override;

   protected:
    void press_action() override;
    SamsungClimateUart* samsung_climate_uart_;
};

}  // namespace samsung_ac
}  // namespace esphome