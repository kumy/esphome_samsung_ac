#include "samsung_climate.h"

#include "esphome/core/log.h"
#include "samsung_climate_mode.h"
#include "samsung_proto.h"

namespace esphome {
namespace samsung_ac {

using namespace esphome::climate;

static const int RECEIVE_TIMEOUT = 200;
static const int COMMAND_DELAY = 300;

/**
 * Send the command to UART interface.
 */
void SamsungClimateUart::send_to_uart(SamsungProto command) {
    this->last_command_timestamp_ = millis();
    ESP_LOGV(TAG, "Sending              [%s]", command.GetFormattedMessage().c_str());
    this->write_array(command.GetFullMessageBytes());
}

int8_t SamsungClimateUart::validate_message_() {
    uint8_t at = this->rx_message_.size() - 1;
    auto *data = &this->rx_message_[0];

    if (at == 0) {
        if (data[0] == 0xd0) {
            return -1;
        }
        return at + 1;
    }
    if (at < 4) {
        return -1;
    }
    size_t frameLen = data[3];
    size_t expectedSize = 4 + frameLen - 1;
    if (at < expectedSize) {
        return -1;
    }

    SamsungProto message;
    if (!message.ValidateFrame(this->rx_message_)) {
        ESP_LOGW(TAG, "Received invalid message (checksum or format error) DATA=[%s]",
                 format_hex_pretty(data, this->rx_message_.size()).c_str());
        auto it = std::find(this->rx_message_.begin(), this->rx_message_.end(), 0xd0);
        if (it != this->rx_message_.end()) {
            return std::distance(this->rx_message_.begin(), it);
        }
        return this->rx_message_.size();
    }
    message.DecodeFrame(this->rx_message_);
    ESP_LOGV(TAG, "Received message     [%s]", message.GetFormattedMessage().c_str());

    if (!message.IsAck()) {
        this->enqueue_command_(message.Ack());
        this->parseResponse(message);
    } else if (message.isMessageType((uint8_t)0x12) || message.isMessageType((uint16_t)0x1303) || message.isMessageType((uint16_t)0x1403)) {
        this->parseResponse(message);
    }

    return message.getMessageLen();
}

void SamsungClimateUart::enqueue_command_(const SamsungProto &command) {
    ESP_LOGV(TAG, "Enqueue SamsungProto [%s]", command.GetFormattedMessage().c_str());
    // if (this->command_queue_ == nullptr) {
    //     ESP_LOGW(TAG, "command_queue_ is NULL");
    //     return;
    // }
    this->command_queue_.push_back(command);
    // this->process_command_queue_();
    ESP_LOGV(TAG, "Queue len %d", this->command_queue_.size());
}

void SamsungClimateUart::sendCmd(uint16_t messageType, const std::vector<std::pair<uint8_t, std::vector<uint8_t>>> &registers) {
    SamsungProto::Builder protoBuilder;
    protoBuilder.SetMessageType(messageType)
        .AddRegisters(registers);
    SamsungProto proto = protoBuilder.Build();
    this->enqueue_command_(proto);
}

void SamsungClimateUart::getInitData() {
    ESP_LOGD(TAG, "Requesting initial data from AC unit");

    // d0c002 12 0000000000 00 fe 1204 06 01010f 7401f0 64 e0
    // d0c002 0e 0000000000 00 fe 1204 02 01010f 7401f0 f6 e0
    // d0c00220000000000008fe120214 0100 0200 4300 5a00 4400 f700 5c00 7300 6200 6300 47 e0
    // d0c0021c000000000008fe130210 3200 4000 4400 4300 7500 7600 7700 7800 80 e0
    // d0c00222000000000009fe140216 3200 f600 f400 f300 f500 3900 e000 e400 e800 e900 e600 2b e0
    this->sendCmd(0x1204,
                  {{0x01, {0x0f}}, {0x74, {0xf0}}});
    this->sendCmd(0x1202,
                  {{0x01, {}}, {0x02, {}}, {0x43, {}}, {0x5a, {}}, {0x44, {}}, {0xf7, {}}, {0x5c, {}}, {0x73, {}}, {0x62, {}}, {0x63, {}}});
    this->sendCmd(0x1302,
                  {{0x32, {}}, {0x40, {}}, {0x44, {}}, {0x43, {}}, {0x75, {}}, {0x76, {}}, {0x77, {}}, {0x78, {}}});
    this->sendCmd(0x1402,
                  {{0x32, {}}, {0xf6, {}}, {0xf4, {}}, {0xf3, {}}, {0xf5, {}}, {0x39, {}}, {0xe0, {}}, {0xe4, {}}, {0xe8, {}}, {0xe9, {}}, {0xe6, {}}});
}

void SamsungClimateUart::setup() {
    // load initial sensor data from the unit
    this->getInitData();
}

/**
 * Detect RX timeout and send next command in the queue to the unit.
 */
void SamsungClimateUart::process_command_queue_() {
    uint32_t now = millis();
    uint32_t cmdDelay = now - this->last_command_timestamp_;

    // when we have not processed message and timeout since last received byte has expired,
    // we likely won't receive any more data and there is nothing we can do with the message as it's
    // format is was not recognized by validate_message_ function.
    // Nothing to do - drop the message to free up communication and allow to send next command.
    if (now - this->last_rx_char_timestamp_ > RECEIVE_TIMEOUT) {
        this->rx_message_.clear();
    }

    // when there is no RX message and there is a command to send
    if (cmdDelay > COMMAND_DELAY && !this->command_queue_.empty() && this->rx_message_.empty()) {
        auto newCommand = this->command_queue_.front();
        // if (newCommand.cmd == SamsungCommandType::DELAY && cmdDelay < newCommand.delay) {
        //   // delay command did not finished yet
        //   return;
        // }
        this->send_to_uart(this->command_queue_.front());
        this->command_queue_.erase(this->command_queue_.begin());
        ESP_LOGV(TAG, "Queue len %d", this->command_queue_.size());
    }
}

/**
 * Handle received byte from UART
 */
void SamsungClimateUart::handle_rx_byte_(uint8_t c) {
    this->rx_message_.push_back(c);
    int8_t consumed = validate_message_();
    if (consumed > -1) {
        if (consumed >= this->rx_message_.size()) {
            this->rx_message_.clear();
            return;
        }
        this->rx_message_.erase(this->rx_message_.begin(), this->rx_message_.begin() + consumed);
    } else {
        this->last_rx_char_timestamp_ = millis();
    }
}

void SamsungClimateUart::loop() {
    if (available()) {
        uint8_t c;
        this->read_byte(&c);
        this->handle_rx_byte_(c);
    }
    this->process_command_queue_();
}

void SamsungClimateUart::parseResponse(const SamsungProto &command) {
    esphome::climate::ClimateMode opMode;
    esphome::climate::ClimateSwingMode swingMode;
    esphome::samsung_ac::STATE climateState;
    esphome::samsung_ac::CO_MODE coMode;
    int8_t tempCelcius;

    for (auto it = command.getRegisterIterator(); it != command.getRegisterEndIterator(); ++it) {
        const SamsungProto::Register &reg = *it;
        ESP_LOGV(TAG, "Received register 0x%02x with value 0x%s and len %d", reg.id, command.format_hex_pretty(reg.value).c_str(), (int)reg.len);
        switch (command.getMessageType()) {
            case 0x1203:
            case 0x1205:
            case 0x1206:
                switch (reg.id) {
                    case 0x01:  // FUN_ENABLE
                        ESP_LOGD(TAG, "Received FUN_ENABLE: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x02:  // FUN_POWER
                        ESP_LOGD(TAG, "Received FUN_POWER: %s", command.format_hex_pretty(reg.value).c_str());
                        climateState = static_cast<STATE>(reg.value[0]);
                        ESP_LOGD(TAG, "Received AC unit power state: %s", climate_state_to_string(climateState));
                        if (climateState == STATE::OFF) {
                            // AC unit was just powered off, set mode to OFF
                            this->mode = climate::CLIMATE_MODE_OFF;
                        } else if (this->mode == climate::CLIMATE_MODE_OFF && climateState == STATE::ON) {
                            // AC unit was just powered on, query unit for it's MODE
                            this->getInitData();
                        }
                        this->power_state_ = climateState;
                        break;
                    case 0x41:  // FUN_UNKNOWN_41
                        ESP_LOGD(TAG, "Received FUN_UNKNOWN_41: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x43:  // FUN_OPMODE
                        ESP_LOGD(TAG, "Received FUN_OPMODE: %s", command.format_hex_pretty(reg.value).c_str());
                        opMode = IntToClimateMode(static_cast<MODE>(reg.value[0]));
                        ESP_LOGD(TAG, "Received AC mode: %s", climate_mode_to_string(opMode));
                        if (this->power_state_ == STATE::ON) {
                            this->mode = opMode;
                        }
                        break;
                    case 0x44:  // FUN_COMODE
                        ESP_LOGD(TAG, "Received FUN_COMODE: %s", command.format_hex_pretty(reg.value).c_str());

                        coMode = static_cast<CO_MODE>(reg.value.front());
                        ESP_LOGI(TAG, "Received co mode: %s", IntToCoModeStr(coMode).c_str());
                        if (this->co_mode_select_ != nullptr) {
                            this->co_mode_select_->publish_state(IntToCoModeStr(coMode));
                        }

                        break;
                    case 0x5a:  // FUN_TEMP_SET
                        ESP_LOGD(TAG, "Received target temp: %d", (int8_t)reg.value.front());
                        this->target_temperature = (int8_t)reg.value.front();
                        break;
                    case 0x5c:  // FUN_TEMP_NOW
                        ESP_LOGD(TAG, "Received room temp: %d °C", (int8_t)reg.value.front());
                        this->current_temperature = (int8_t)reg.value.front();
                        break;
                    case 0x62:  // FUN_WINDLEVEL
                        ESP_LOGD(TAG, "Received FUN_WINDLEVEL: %s", command.format_hex_pretty(reg.value).c_str());
                        if (static_cast<FAN>(reg.value[0]) == FAN::TURBO) {
                            auto fanMode = IntToCustomFanMode(static_cast<FAN>(reg.value[0]));
                            ESP_LOGD(TAG, "Received custom fan mode: [%02x] %s", reg.value[0], fanMode.c_str());
                            this->set_custom_fan_mode_(fanMode);
                        } else {
                            auto fanMode = IntToClimateFanMode(static_cast<FAN>(reg.value[0]));
                            ESP_LOGD(TAG, "Received fan mode: [%02x] %s", reg.value[0], climate_fan_mode_to_string(fanMode));
                            this->set_fan_mode_(fanMode);
                        }
                        break;
                    case 0x63:  // FUN_DIRECTION
                        ESP_LOGD(TAG, "Received FUN_DIRECTION: %s", command.format_hex_pretty(reg.value).c_str());
                        swingMode = IntToClimateSwingMode(static_cast<SWING>(reg.value[0]));
                        this->swing_mode = swingMode;
                        ESP_LOGD(TAG, "Received swing mode: %s", climate_swing_mode_to_string(swingMode));
                        break;
                    case 0x73:  // FUN_SLEEP
                        ESP_LOGD(TAG, "Received FUN_SLEEP: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x74:  // FUN_UNKNOWN_74
                        ESP_LOGD(TAG, "Received FUN_UNKNOWN_74: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xea:  // FUN_UNKNOWN_EA
                        ESP_LOGD(TAG, "Received FUN_UNKNOWN_EA: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xf7:  // FUN_ERROR
                        ESP_LOGD(TAG, "Received FUN_ERROR: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    default:
                        ESP_LOGW(TAG, "Received unknown %04x register [%02x] with length: %d and value %s",
                                 command.getMessageType(), reg.id, (int)reg.len, command.format_hex_pretty(reg.value).c_str());
                        break;
                }
                break;
            case 0x1303:
            case 0x1306:
                switch (reg.id) {
                    case 0x32:  // AC_ADD_AUTOCLEAN
                        ESP_LOGD(TAG, "Received AC_ADD_AUTOCLEAN: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x40:  // AC_ADD_SETKWH
                        ESP_LOGD(TAG, "Received AC_ADD_SETKWH: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x44:  // AC_ADD_CLEAR_FILTER_ALARM
                        ESP_LOGD(TAG, "Received AC_ADD_CLEAR_FILTER_ALARM: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x43:  // AC_ADD_STARTWPS
                        ESP_LOGD(TAG, "Received AC_ADD_STARTWPS: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x75:  // AC_ADD_SPI
                        ESP_LOGD(TAG, "Received AC_ADD_SPI: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x76:  // AC_OUTDOOR_TEMP
                        tempCelcius = ((int8_t)reg.value.front() - 31) / 1.8;
                        ESP_LOGD(TAG, "Received outdoor temp: %d °C", tempCelcius);
                        if (this->outdoor_temp_sensor_ != nullptr) {
                            this->outdoor_temp_sensor_->publish_state(tempCelcius);
                        }
                        break;
                    case 0x77:  // AC_COOL_CAPABILITY
                        ESP_LOGD(TAG, "Received AC_COOL_CAPABILITY: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x78:  // AC_WARM_CAPABILITY
                        ESP_LOGD(TAG, "Received AC_WARM_CAPABILITY: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    default:
                        ESP_LOGW(TAG, "Received unknown %04x register [%02x] with length: %d and value %s",
                                 command.getMessageType(), reg.id, (int)reg.len, command.format_hex_pretty(reg.value).c_str());
                        break;
                }
                break;
            case 0x1404:
                switch (reg.id) {
                    case 0x17:  // AC_ADD2_UNKNOWN_17
                        ESP_LOGD(TAG, "Received AC_ADD2_UNKNOWN_17: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x18:  // AC_ADD2_UNKNOWN_18
                        ESP_LOGD(TAG, "Received AC_ADD2_UNKNOWN_18: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x19:  // AC_ADD2_UNKNOWN_19
                        ESP_LOGD(TAG, "Received AC_ADD2_UNKNOWN_19: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xf7:  // AC_SG_MACHIGH
                        ESP_LOGD(TAG, "Received AC_SG_MACHIGH: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xf8:  // AC_SG_MACMID
                        ESP_LOGD(TAG, "Received AC_SG_MACMID: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xf9:  // AC_SG_MACLOW
                        ESP_LOGD(TAG, "Received AC_SG_MACLOW: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xfa:  // AC_SG_VENDER01
                        ESP_LOGD(TAG, "Received AC_SG_VENDER01: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xfb:  // AC_SG_VENDER02
                        ESP_LOGD(TAG, "Received AC_SG_VENDER02: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xfc:  // AC_SG_VENDER03
                        ESP_LOGD(TAG, "Received AC_SG_VENDER03: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xfd:  // AC_ADD2_UNKNOWN_FD
                        ESP_LOGD(TAG, "Received AC_ADD2_UNKNOWN_FD: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    default:
                        ESP_LOGW(TAG, "Received unknown %04x register [%02x] with length: %d and value %s",
                                 command.getMessageType(), reg.id, (int)reg.len, command.format_hex_pretty(reg.value).c_str());
                        break;
                }
                break;
            case 0x1403:
            case 0x1406:
                switch (reg.id) {
                    case 0x32:  // AC_ADD2_USEDWATT
                        ESP_LOGD(TAG, "Received AC_ADD2_USEDWATT: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xf6:  // AC_ADD2_VERSION
                        ESP_LOGD(TAG, "Received AC_ADD2_VERSION: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xf4:  // AC_ADD2_PANEL_VERSION
                        ESP_LOGD(TAG, "Received AC_ADD2_PANEL_VERSION: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xf3:  // AC_ADD2_OUT_VERSION
                        ESP_LOGD(TAG, "Received AC_ADD2_OUT_VERSION: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xf5:  // AC_FUN_MODEL
                        ESP_LOGD(TAG, "Received AC_FUN_MODEL: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0x39:  // AC_ADD2_OPTIONCODE
                        ESP_LOGD(TAG, "Received AC_ADD2_OPTIONCODE: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xe0:  // AC_ADD2_USEDPOWER
                        ESP_LOGD(TAG, "Received AC_ADD2_USEDPOWER: %s", command.format_hex_pretty(reg.value).c_str());
                        if (this->used_power_sensor_ != nullptr) {
                            // Apparently, the counter increments by 1 every 500W
                            this->used_power_sensor_->publish_state(vectorToInt(reg.value) * 0.5);
                            // this->used_power_sensor_->publish_state(vectorToInt(reg.value));
                        }
                        break;
                    case 0xe4:  // AC_ADD2_USEDTIME
                        ESP_LOGD(TAG, "Received AC_ADD2_USEDTIME: %s", command.format_hex_pretty(reg.value).c_str());
                        if (this->runtime_hours_sensor_ != nullptr) {
                            // The counter increments by 5 every 30 minutes
                            this->runtime_hours_sensor_->publish_state(vectorToInt(reg.value) * 0.1);
                        }
                        break;
                    case 0xe8:  // AC_ADD2_CLEAR_POWERTIME
                        ESP_LOGD(TAG, "Received AC_ADD2_CLEAR_POWERTIME: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xe9:  // AC_ADD2_FILTERTIME
                        ESP_LOGD(TAG, "Received AC_ADD2_FILTERTIME: %s", command.format_hex_pretty(reg.value).c_str());
                        break;
                    case 0xe6:  // AC_ADD2_FILTER_USE_TIME
                        ESP_LOGD(TAG, "Received AC_ADD2_FILTER_USE_TIME: %s", command.format_hex_pretty(reg.value).c_str());
                        if (this->filtertime_hours_sensor_ != nullptr) {
                            // The counter increments by 5 every 30 minutes
                            this->filtertime_hours_sensor_->publish_state(vectorToInt(reg.value) * 0.1);
                        }
                        break;
                    default:
                        ESP_LOGW(TAG, "Received unknown %04x register [%02x] with length: %d and value %s",
                                 command.getMessageType(), reg.id, (int)reg.len, command.format_hex_pretty(reg.value).c_str());
                        break;
                }
                break;
            default:
                ESP_LOGW(TAG, "Received unknown %04x command type [%s]",
                         command.getMessageType(), command.GetFormattedMessage().c_str());
                break;
        }
    }
    this->publish_state();
}

uint32_t vectorToInt(const std::vector<uint8_t> &vec) {
    if (vec.size() > sizeof(uint32_t)) {
        ESP_LOGW(TAG, "Fail to convert vector to int");
        return -1;
    }

    uint32_t result = 0;
    for (size_t i = 0; i < vec.size(); ++i) {
        result = (result << 8) | vec[i];  // Shift left and add the next byte
    }

    return result;
}

void SamsungClimateUart::dump_config() {
    ESP_LOGCONFIG(TAG, "SamsungClimate:");
    LOG_CLIMATE("", "Thermostat", this);
    if (this->outdoor_temp_sensor_ != nullptr) {
        LOG_SENSOR("", "Outdoor Temp", this->outdoor_temp_sensor_);
    }
    if (this->co_mode_select_ != nullptr) {
        LOG_SELECT("", "Co mode selector", this->co_mode_select_);
    }
    ESP_LOGI(TAG, "Min Temp: %d", this->min_temp_);
}

/**
 * Periodically send fake Wifi and internet status.
 */
void SamsungClimateUart::update() {
    this->sendCmd(0x1404, {{0x37, {0x0f}}});
    this->sendCmd(0x1404, {{0x38, {0x0f}}});
}

void SamsungClimateUart::control(const climate::ClimateCall &call) {
    if (call.get_mode().has_value()) {
        ClimateMode mode = *call.get_mode();
        ESP_LOGD(TAG, "Setting mode to %s", climate_mode_to_string(mode));
        if (this->mode == CLIMATE_MODE_OFF && mode != CLIMATE_MODE_OFF) {
            ESP_LOGD(TAG, "Setting AC unit power state to ON.");
            this->sendCmd(0x1204, {{0x02, {0x0f}}});
        }
        if (mode == CLIMATE_MODE_OFF) {
            ESP_LOGD(TAG, "Setting AC unit power state to OFF.");
            this->sendCmd(0x1204, {{0x02, {0xf0}}});
        } else {
            uint8_t requestedMode = (uint8_t)ClimateModeToInt(mode);
            this->sendCmd(0x1204, {{0x43, {requestedMode}}});
        }
        this->mode = mode;
    }

    if (call.get_target_temperature().has_value()) {
        auto target_temp = *call.get_target_temperature();
        uint8_t intTemp = (uint8_t)target_temp;
        ESP_LOGD(TAG, "Setting target temp to %d", intTemp);
        this->target_temperature = target_temp;
        this->sendCmd(0x1204, {{0x5a, {intTemp}}});
    }

    if (call.get_fan_mode().has_value()) {
        auto fan_mode = *call.get_fan_mode();
        uint8_t mode = (uint8_t)ClimateFanModeToInt(fan_mode);
        ESP_LOGW(TAG, "Setting fan mode to %s", climate_fan_mode_to_string(fan_mode));
        this->set_fan_mode_(fan_mode);
        this->sendCmd(0x1204, {{0x62, {mode}}});
    }

    if (call.get_custom_fan_mode().has_value()) {
        auto fan_mode = *call.get_custom_fan_mode();
        auto payload = StringToFanLevel(fan_mode);
        if (payload.has_value()) {
            ESP_LOGW(TAG, "Setting custom fan mode to %s [%02x]", fan_mode.c_str(), payload.value());
            this->set_custom_fan_mode_(fan_mode);
            this->sendCmd(0x1204, {{0x62, {static_cast<uint8_t>(payload.value())}}});
        }
    }

    if (call.get_swing_mode().has_value()) {
        auto swing_mode = *call.get_swing_mode();
        uint8_t function_value = (uint8_t)ClimateSwingModeToInt(swing_mode);
        ESP_LOGD(TAG, "Setting swing mode to %s", climate_swing_mode_to_string(swing_mode));
        this->swing_mode = swing_mode;
        this->sendCmd(0x1204, {{0x63, {function_value}}});
    }

    this->publish_state();
}

ClimateTraits SamsungClimateUart::traits() {
    auto traits = climate::ClimateTraits();
    traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT_COOL, climate::CLIMATE_MODE_COOL,
                                climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_DRY, climate::CLIMATE_MODE_FAN_ONLY});
    if (this->horizontal_swing_) {
        traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                                          climate::CLIMATE_SWING_HORIZONTAL});
    } else {
        traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL});
    }
    traits.set_supports_current_temperature(true);

    traits.add_supported_fan_mode(CLIMATE_FAN_AUTO);
    traits.add_supported_fan_mode(CLIMATE_FAN_LOW);
    traits.add_supported_fan_mode(CLIMATE_FAN_MEDIUM);
    traits.add_supported_fan_mode(CLIMATE_FAN_HIGH);
    // Samsung AC has more FAN levels that standard climate component, we have to use custom.
    traits.add_supported_custom_fan_mode(CUSTOM_FAN_LEVEL_TURBO);

    traits.set_visual_temperature_step(1);
    traits.set_visual_min_temperature(this->min_temp_);
    traits.set_visual_max_temperature(MAX_TEMP);
    return traits;
}

void SamsungClimateUart::on_set_co_mode(const std::string &value) {
  auto co_mode = CoModeStrToInt(value);
  ESP_LOGD(TAG, "Setting co mode to %s [%02x]", value.c_str(), co_mode.value());
  // this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(special_mode.value()));
  this->sendCmd(0x1204, {{0x44, {co_mode.value()}}});
  this->co_mode_select_->publish_state(value);
}

void SamsungCoModeSelect::control(const std::string &value) { parent_->on_set_co_mode(value); }

}  // namespace samsung_ac
}  // namespace esphome
