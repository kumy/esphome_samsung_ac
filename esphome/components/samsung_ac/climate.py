import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, sensor, climate, uart, select, button
from esphome.const import (
    CONF_ID,
    CONF_BUTTON,
    DEVICE_CLASS_BUTTON,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_POWER,
    ICON_TIMER,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_CELSIUS,
    UNIT_HOUR,
    UNIT_KILOWATT_HOURS,
)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "select"]

CONF_ROOM_TEMP = "room_temp"
CONF_OUTDOOR_TEMP = "outdoor_temp"
CONF_CO_MODE = "co_mode"
CONF_CO_MODE_MODES = "modes"

FEATURE_HORIZONTAL_SWING = "horizontal_swing"
MIN_TEMP = "min_temp"

CONF_RUNTIME_HOURS_SENSOR = "runtime_hours_sensor"
CONF_FILTERTIME_HOURS_SENSOR = "filtertime_hours_sensor"
CONF_USEDPOWER_SENSOR = "used_power_sensor"
CONF_RESET_USED_POWER_BUTTON = "reset_used_power_button"

# CONF_SUPPORTS = "supports"
# DEFAULT_CLIMATE_MODES = ["AUTO", "COOL", "HEAT", "DRY", "FAN_ONLY", "HEAT_COOL"]
# DEFAULT_FAN_MODES = ["AUTO", "LOW", "MEDIUM", "HIGH"]
# DEFAULT_SWING_MODES = ["OFF", "VERTICAL", "HORIZONTAL", "BOTH"]

ICON_RESTART = "mdi:restart"
ICON_FLASH = "mdi:flash"

samsung_ns = cg.esphome_ns.namespace("samsung_ac")
SamsungClimateUart = samsung_ns.class_("SamsungClimateUart", cg.PollingComponent, button.Button, climate.Climate, uart.UARTDevice)

SamsungCoModeSelect = samsung_ns.class_('SamsungCoModeSelect', select.Select)
ResetUsedPowerButton = samsung_ns.class_("ResetUsedPowerButton", button.Button, cg.Component)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SamsungClimateUart),
        cv.Optional(CONF_OUTDOOR_TEMP): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        cv.Optional(FEATURE_HORIZONTAL_SWING): cv.boolean,
        cv.Optional(MIN_TEMP): cv.int_,

        cv.Optional(CONF_RUNTIME_HOURS_SENSOR): sensor.sensor_schema(
                unit_of_measurement=UNIT_HOUR,
                icon=ICON_TIMER,
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
                device_class=DEVICE_CLASS_DURATION,
                # entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),

        cv.Optional(CONF_FILTERTIME_HOURS_SENSOR): sensor.sensor_schema(
                unit_of_measurement=UNIT_HOUR,
                icon=ICON_TIMER,
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
                device_class=DEVICE_CLASS_DURATION,
                # entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),

        cv.Optional(CONF_USEDPOWER_SENSOR): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                icon=ICON_FLASH,
                accuracy_decimals=1,
                state_class=STATE_CLASS_TOTAL_INCREASING,
                device_class=DEVICE_CLASS_ENERGY,
                # entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),

        cv.Optional(CONF_RESET_USED_POWER_BUTTON): button.button_schema(
                ResetUsedPowerButton,
                icon=ICON_RESTART,
                entity_category=ENTITY_CATEGORY_CONFIG,
            ),

        cv.Optional(CONF_CO_MODE): select.SELECT_SCHEMA.extend({
            cv.GenerateID(): cv.declare_id(SamsungCoModeSelect),
            cv.Required(CONF_CO_MODE_MODES): cv.ensure_list(cv.one_of("Off","TurboMode","Smart","Sleep","Quiet","SoftCool", "WindMode1","WindMode2","WindMode3"))
        }),
    }
).extend(uart.UART_DEVICE_SCHEMA).extend(cv.polling_component_schema("120s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)

    if CONF_OUTDOOR_TEMP in config:
        conf = config[CONF_OUTDOOR_TEMP]
        sensor_ = await sensor.new_sensor(conf)
        cg.add(var.set_outdoor_temp_sensor(sensor_))

    if FEATURE_HORIZONTAL_SWING in config:
        cg.add(var.set_horizontal_swing(True))

    if MIN_TEMP in config:
        cg.add(var.set_min_temp(config[MIN_TEMP]))

    if CONF_RUNTIME_HOURS_SENSOR in config:
        conf = config[CONF_RUNTIME_HOURS_SENSOR]
        conf["force_update"] = False
        sensor_ = await sensor.new_sensor(conf)
        cg.add(var.set_runtime_hours_sensor(sensor_))

    if CONF_FILTERTIME_HOURS_SENSOR in config:
        conf = config[CONF_FILTERTIME_HOURS_SENSOR]
        conf["force_update"] = False
        sensor_ = await sensor.new_sensor(conf)
        cg.add(var.set_filtertime_hours_sensor(sensor_))

    if CONF_USEDPOWER_SENSOR in config:
        conf = config[CONF_USEDPOWER_SENSOR]
        conf["force_update"] = False
        sensor_ = await sensor.new_sensor(conf)
        cg.add(var.set_used_power_sensor(sensor_))

    if CONF_RESET_USED_POWER_BUTTON in config:
        conf = config[CONF_RESET_USED_POWER_BUTTON]
        btn_ = await button.new_button(conf)
        cg.add(btn_.set_samsung_climat_uart(var))

    if CONF_CO_MODE in config:
        sel = await select.new_select(config[CONF_CO_MODE], options=config[CONF_CO_MODE][CONF_CO_MODE_MODES])
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_co_mode_select(sel))
