#include "samsung_climate_mode.h"

#include "esphome/components/climate/climate.h"
#include "esphome/core/log.h"
#include "samsung_climate.h"

namespace esphome {
namespace samsung_ac {

const MODE ClimateModeToInt(climate::ClimateMode mode) {
    switch (mode) {
        case climate::CLIMATE_MODE_HEAT_COOL:
            return MODE::HEAT_COOL;
        case climate::CLIMATE_MODE_COOL:
            return MODE::COOL;
        case climate::CLIMATE_MODE_HEAT:
            return MODE::HEAT;
        case climate::CLIMATE_MODE_DRY:
            return MODE::DRY;
        case climate::CLIMATE_MODE_FAN_ONLY:
            return MODE::FAN_ONLY;
        default:
            ESP_LOGE(TAG, "Invalid climate mode.");
            return MODE::HEAT_COOL;
    }
}

const climate::ClimateMode IntToClimateMode(MODE mode) {
    switch (mode) {
        case MODE::HEAT_COOL:
            return climate::CLIMATE_MODE_HEAT_COOL;
        case MODE::COOL:
            return climate::CLIMATE_MODE_COOL;
        case MODE::HEAT:
            return climate::CLIMATE_MODE_HEAT;
        case MODE::DRY:
            return climate::CLIMATE_MODE_DRY;
        case MODE::FAN_ONLY:
            return climate::CLIMATE_MODE_FAN_ONLY;
        default:
            ESP_LOGE(TAG, "Invalid climate mode.");
            return climate::CLIMATE_MODE_OFF;
    }
}

const SWING ClimateSwingModeToInt(climate::ClimateSwingMode mode) {
    switch (mode) {
        case climate::CLIMATE_SWING_OFF:
            return SWING::FIXED;
        case climate::CLIMATE_SWING_BOTH:
            return SWING::ROTATION;
        case climate::CLIMATE_SWING_VERTICAL:
            return SWING::VERTICAL;
        case climate::CLIMATE_SWING_HORIZONTAL:
            return SWING::HORIZONTAL;
        default:
            ESP_LOGE(TAG, "Invalid swing mode %d.", mode);
            return SWING::FIXED;
    }
}

const climate::ClimateSwingMode IntToClimateSwingMode(SWING mode) {
    switch (mode) {
        case SWING::OFF:
        case SWING::FIXED:
            return climate::CLIMATE_SWING_OFF;
        case SWING::VERTICAL:
            return climate::CLIMATE_SWING_VERTICAL;
        case SWING::HORIZONTAL:
            return climate::CLIMATE_SWING_HORIZONTAL;
        case SWING::ROTATION:
            return climate::CLIMATE_SWING_BOTH;
        default:
            ESP_LOGE(TAG, "Invalid swing mode %d.", mode);
            return climate::CLIMATE_SWING_OFF;
    }
}

const FAN ClimateFanModeToInt(climate::ClimateFanMode mode) {
    switch (mode) {
        case climate::CLIMATE_FAN_AUTO:
            return FAN::AUTO;
        case climate::CLIMATE_FAN_LOW:
            return FAN::LOW;
        case climate::CLIMATE_FAN_MEDIUM:
            return FAN::MID;
        case climate::CLIMATE_FAN_HIGH:
            return FAN::HIGH;
        default:
            ESP_LOGE(TAG, "Invalid fan mode %d.", mode);
            return FAN::AUTO;
    }
}
const climate::ClimateFanMode IntToClimateFanMode(FAN mode) {
    switch (mode) {
        case FAN::AUTO:
            return climate::CLIMATE_FAN_AUTO;
        case FAN::LOW:
            return climate::CLIMATE_FAN_LOW;
        case FAN::MID:
            return climate::CLIMATE_FAN_MEDIUM;
        case FAN::HIGH:
            return climate::CLIMATE_FAN_HIGH;
        default:
            ESP_LOGE(TAG, "Invalid fan mode %d.", mode);
            return climate::CLIMATE_FAN_AUTO;
    }
}

const optional<FAN> StringToFanLevel(std::string mode) {
    if (mode == CUSTOM_FAN_LEVEL_TURBO) {
        return FAN::TURBO;
    }
    return nullopt;
}

const std::string IntToCustomFanMode(FAN mode) {
    switch (mode) {
        case FAN::TURBO:
            return CUSTOM_FAN_LEVEL_TURBO;
        default:
            return "Unknown";
    }
}

const LogString *climate_state_to_string(STATE mode) {
    switch (mode) {
        case STATE::ON:
            return LOG_STR("ON");
        case STATE::OFF:
            return LOG_STR("OFF");
        default:
            return LOG_STR("UNKNOWN");
    }
}

const optional<CO_MODE> CoModeStrToInt(std::string mode) {
    if (str_equals_case_insensitive(mode, CO_MODE_OFF)) {
        return CO_MODE::OFF;
    } else if (str_equals_case_insensitive(mode, CO_MODE_TURBO)) {
        return CO_MODE::TURBO;
    } else if (str_equals_case_insensitive(mode, CO_MODE_SMART)) {
        return CO_MODE::SMART;
    } else if (str_equals_case_insensitive(mode, CO_MODE_SLEEP)) {
        return CO_MODE::SLEEP;
    } else if (str_equals_case_insensitive(mode, CO_MODE_QUIET)) {
        return CO_MODE::QUIET;
    } else if (str_equals_case_insensitive(mode, CO_MODE_SOFTCOOL)) {
        return CO_MODE::SOFTCOOL;
    } else if (str_equals_case_insensitive(mode, CO_MODE_WINDMODE1)) {
        return CO_MODE::WINDMODE1;
    } else if (str_equals_case_insensitive(mode, CO_MODE_WINDMODE2)) {
        return CO_MODE::WINDMODE2;
    } else if (str_equals_case_insensitive(mode, CO_MODE_WINDMODE3)) {
        return CO_MODE::WINDMODE3;
    } else {
        return CO_MODE::OFF;
    }
}

const std::string IntToCoModeStr(CO_MODE mode) {
    switch (mode) {
        case CO_MODE::OFF:
            return CO_MODE_OFF;
        case CO_MODE::TURBO:
            return CO_MODE_TURBO;
        case CO_MODE::SMART:
            return CO_MODE_SMART;
        case CO_MODE::SLEEP:
            return CO_MODE_SLEEP;
        case CO_MODE::QUIET:
            return CO_MODE_QUIET;
        case CO_MODE::SOFTCOOL:
            return CO_MODE_SOFTCOOL;
        case CO_MODE::WINDMODE1:
            return CO_MODE_WINDMODE1;
        case CO_MODE::WINDMODE2:
            return CO_MODE_WINDMODE2;
        case CO_MODE::WINDMODE3:
            return CO_MODE_WINDMODE3;
        default:
            return "Unknown";
    }
}

const optional<ENABLE> EnableStrToInt(std::string enable) {
    if (str_equals_case_insensitive(enable, ENABLE_OFF)) {
        return ENABLE::OFF;
    } else if (str_equals_case_insensitive(enable, ENABLE_ON)) {
        return ENABLE::ON;
    } else {
        return ENABLE::OFF;
    }
}
const std::string IntToEnableStr(ENABLE enable) {
    switch (enable) {
        case ENABLE::OFF:
            return ENABLE_OFF;
        case ENABLE::ON:
            return ENABLE_ON;
        default:
            return "Unknown";
    }
}

}  // namespace samsung_ac
}  // namespace esphome
