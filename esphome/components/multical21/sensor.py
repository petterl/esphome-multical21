import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_WATER,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_TOTAL_INCREASING,
    STATE_CLASS_MEASUREMENT,
    UNIT_CUBIC_METER,
    UNIT_CELSIUS,
)
from . import multical21_ns, Multical21Component

CONF_MULTICAL21_ID = "multical21_id"
CONF_TOTAL_CONSUMPTION = "total_consumption"
CONF_MONTH_START_VALUE = "month_start_value"
CONF_WATER_TEMPERATURE = "water_temperature"
CONF_AMBIENT_TEMPERATURE = "ambient_temperature"
CONF_CURRENT_FLOW = "current_flow"
CONF_DAILY_CONSUMPTION = "daily_consumption"

# Unit constants not in esphome.const
UNIT_LITERS = "L"
UNIT_LITERS_PER_HOUR = "L/h"

DEPENDENCIES = ["multical21"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MULTICAL21_ID): cv.use_id(Multical21Component),
        cv.Optional(CONF_TOTAL_CONSUMPTION): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_MONTH_START_VALUE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_WATER_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AMBIENT_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CURRENT_FLOW): sensor.sensor_schema(
            unit_of_measurement=UNIT_LITERS_PER_HOUR,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DAILY_CONSUMPTION): sensor.sensor_schema(
            unit_of_measurement=UNIT_LITERS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_MULTICAL21_ID])

    if CONF_TOTAL_CONSUMPTION in config:
        sens = await sensor.new_sensor(config[CONF_TOTAL_CONSUMPTION])
        cg.add(parent.set_total_consumption_sensor(sens))

    if CONF_MONTH_START_VALUE in config:
        sens = await sensor.new_sensor(config[CONF_MONTH_START_VALUE])
        cg.add(parent.set_month_start_sensor(sens))

    if CONF_WATER_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_WATER_TEMPERATURE])
        cg.add(parent.set_water_temp_sensor(sens))

    if CONF_AMBIENT_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_AMBIENT_TEMPERATURE])
        cg.add(parent.set_ambient_temp_sensor(sens))

    if CONF_CURRENT_FLOW in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_FLOW])
        cg.add(parent.set_current_flow_sensor(sens))

    if CONF_DAILY_CONSUMPTION in config:
        sens = await sensor.new_sensor(config[CONF_DAILY_CONSUMPTION])
        cg.add(parent.set_daily_consumption_sensor(sens))
