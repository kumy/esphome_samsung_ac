#pragma once

#include <cstdint>
#include "esphome/core/log.h"
#include "esphome/components/climate/climate.h"

namespace esphome
{
  namespace samsung_ac
  {

    // static const std::string &CUSTOM_FAN_LEVEL_AUTO = "Auto";
    // static const std::string &CUSTOM_FAN_LEVEL_1 = "Low";
    // static const std::string &CUSTOM_FAN_LEVEL_2 = "Mid";
    // static const std::string &CUSTOM_FAN_LEVEL_3 = "High";
    static const std::string &CUSTOM_FAN_LEVEL_TURBO = "Turbo";

    static const std::string &CO_MODE_OFF = "Off";
    static const std::string &CO_MODE_TURBO = "Turbo";
    static const std::string &CO_MODE_SMART = "Smart";
    static const std::string &CO_MODE_SLEEP = "Sleep";
    static const std::string &CO_MODE_QUIET = "Quiet";
    static const std::string &CO_MODE_SOFTCOOL = "SoftCool";
    static const std::string &CO_MODE_WINDMODE1 = "WindMode1";
    static const std::string &CO_MODE_WINDMODE2 = "WindMode2";
    static const std::string &CO_MODE_WINDMODE3 = "WindMode3";

    static const std::string &ENABLE_ON = "On";
    static const std::string &ENABLE_OFF = "Off";

    enum class CustomFanModes
    {
      // LEVEL_1,
      // LEVEL_2,
      // LEVEL_3,
      TURBO,
      // AUTO
    };

    // enum CO_MODE_STR {
    //   OFF = "Off",
    //   TURBO = "Turbo",
    //   SMART = "Smart",
    //   SLEEP = "Sleep",
    //   QUIET = "Quiet",
    //   SOFTCOOL = "SoftCool",
    //   WINDMODE1 = "WindMode1",
    //   WINDMODE2 = "WindMode2",
    //   WINDMODE3 = "WindMode3",
    // };

    enum CO_MODE: uint8_t {
      OFF = 0x12,
      TURBO = 0x22,
      SMART = 0x32,
      SLEEP = 0x42,
      QUIET = 0x52,
      SOFTCOOL = 0x62,
      WINDMODE1 = 0x82,
      WINDMODE2 = 0x92,
      WINDMODE3 = 0xa2,
    };

    enum class MODE
    {
      OFF = 0x00,
      COOL = 0x12,
      DRY = 0x22,
      FAN_ONLY = 0x32,
      HEAT = 0x42,
      HEAT_COOL = 0xe2
    };
    enum class FAN
    {
      AUTO = 0x00,
      LOW = 0x12,
      MID = 0x14,
      HIGH = 0x16,
      TURBO = 0x18,
    };
    enum class SWING
    {
      OFF = 0x12,
      INDIRECT = 0x21,
      DIRECT = 0x31,
      CENTER = 0x41,
      WIDE = 0x51,
      LEFT = 0x61,
      RIGHT = 0x71,
      LONG = 0x81,
      VERTICAL = 0x92,
      HORIZONTAL = 0xa2,
      ROTATION = 0xb2,
      FIXED = 0xc2,
    };
    enum class STATE
    {
      ON = 0x0f,
      OFF = 0xf0
    };
    enum class ENABLE
    {
      ON = 0x0f,
      OFF = 0xf0
    };

    enum class MessageType : uint16_t
    {
      MSG_1202 = 0x1202,
      MSG_1203 = 0x1203,
      MSG_1204 = 0x1204,
      MSG_1205 = 0x1205,
      MSG_1206 = 0x1206,
      MSG_1207 = 0x1207,
      MSG_1302 = 0x1302,
      MSG_1303 = 0x1303,
      MSG_1304 = 0x1304,
      MSG_1305 = 0x1305,
      MSG_1306 = 0x1306,
      MSG_1307 = 0x1307,
      MSG_1402 = 0x1402,
      MSG_1403 = 0x1403,
      MSG_1404 = 0x1404,
      MSG_1405 = 0x1405,
      MSG_1406 = 0x1406,
      MSG_1407 = 0x1407,
    };

    enum class SamsungCommandType : uint16_t
    {
      MSG_1202 = 0x1202,
    };


    const MODE ClimateModeToInt(climate::ClimateMode mode);
    const climate::ClimateMode IntToClimateMode(MODE mode);

    const SWING ClimateSwingModeToInt(climate::ClimateSwingMode mode);
    const climate::ClimateSwingMode IntToClimateSwingMode(SWING mode);

    const FAN ClimateFanModeToInt(climate::ClimateFanMode mode);
    const climate::ClimateFanMode IntToClimateFanMode(FAN mode);

    const LogString *climate_state_to_string(STATE mode);

    const optional<FAN> StringToFanLevel(std::string mode);
    const ::std::string IntToCustomFanMode(FAN mode);

    const optional<CO_MODE> CoModeStrToInt(std::string mode);
    const std::string IntToCoModeStr(CO_MODE mode);

    const optional<ENABLE> EnableStrToInt(std::string enable);
    const std::string IntToEnableStr(ENABLE enable);


    // const optional<CO_MODE_STR> CoModeStrToInt(const std::string &mode);
    // const std::string IntToCoModeStr(CO_MODE mode);

  } // namespace samsung_ac
} // namespace esphome
