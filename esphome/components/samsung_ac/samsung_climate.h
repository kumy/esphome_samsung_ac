#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/button/button.h"
#include "samsung_climate_mode.h"
#include "samsung_proto.h"

namespace esphome
{
  namespace samsung_ac
  {

    static const char *const TAG = "SamsungClimateUart";
    static const uint8_t MAX_TEMP = 30;

    uint32_t vectorToInt(const std::vector<uint8_t>& vec);

    class SamsungClimateUart : public PollingComponent, public climate::Climate, public uart::UARTDevice
    {
    public:
      void setup() override;
      void loop() override;
      void dump_config() override;
      void update() override;
      float get_setup_priority() const override { return setup_priority::LATE; }

      void set_outdoor_temp_sensor(sensor::Sensor *outdoor_temp_sensor) { this->outdoor_temp_sensor_ = outdoor_temp_sensor; }
      void set_runtime_hours_sensor(sensor::Sensor *runtime_hours_sensor) { this->runtime_hours_sensor_ = runtime_hours_sensor; }
      void set_filtertime_hours_sensor(sensor::Sensor *filtertime_hours_sensor) { this->filtertime_hours_sensor_ = filtertime_hours_sensor; }
      void set_used_power_sensor(sensor::Sensor *used_power_sensor) { this->used_power_sensor_ = used_power_sensor; }
      void set_co_mode_select(select::Select *co_mode_select) { this->co_mode_select_ = co_mode_select; }
      void set_horizontal_swing(bool enabled) { this->horizontal_swing_ = enabled; }
      void set_min_temp(uint8_t min_temp) { this->min_temp_ = min_temp; }

      // void sendCmd(uint16_t messageType, std::vector<SamsungProto::Register> registers);
      void sendCmd(uint16_t messageType, const std::vector<std::pair<uint8_t, std::vector<uint8_t>>> &registers);

    protected:
      /// Override control to change settings of the climate device.
      void control(const climate::ClimateCall &call) override;

      /// Return the traits of this controller.
      climate::ClimateTraits traits() override;

    private:
      std::vector<uint8_t> rx_message_;
      std::vector<SamsungProto> command_queue_;
      uint32_t last_command_timestamp_ = 0;
      uint32_t last_rx_char_timestamp_ = 0;
      STATE power_state_ = STATE::OFF;
      sensor::Sensor *outdoor_temp_sensor_ = nullptr;
      sensor::Sensor *runtime_hours_sensor_ = nullptr;
      sensor::Sensor *filtertime_hours_sensor_ = nullptr;
      sensor::Sensor *used_power_sensor_ = nullptr;
      select::Select *co_mode_select_ = nullptr;
      bool horizontal_swing_ = false;
      uint8_t min_temp_ = 10;

      void enqueue_command_(const SamsungProto &command);
      void send_to_uart(const SamsungProto command);
      void parseResponse(const SamsungProto &command);
      // void requestData(uint8_t[2] messageType, std::vector<Register> registers);
      void process_command_queue_();
      void getInitData();
      void handle_rx_byte_(uint8_t c);
      // bool validate_message_();
      int8_t validate_message_();

      void on_set_co_mode(const std::string &value);
      friend class SamsungCoModeSelect;
    };


    class SamsungCoModeSelect : public select::Select, public esphome::Parented<SamsungClimateUart> {
      protected:
       virtual void control(const std::string &value) override;
     };

  } // namespace samsung_ac
} // namespace esphome
