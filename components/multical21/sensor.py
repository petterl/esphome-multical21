# Multical21 ESPHome Component - Sensor Platform
# Based on work by:
#   Patrik Thalin - https://github.com/pthalin/esp32-multical21
#   Chester - https://github.com/chester4444/esp-multical21

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_WATER,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_COUNTER,
    STATE_CLASS_TOTAL_INCREASING,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_CUBIC_METER,
    UNIT_PERCENT,
)
from . import Multical21Component

CONF_MULTICAL21_ID = "multical21_id"
CONF_TOTAL_CONSUMPTION = "total_consumption"
CONF_MONTH_START_VALUE = "month_start_value"
CONF_WATER_TEMPERATURE = "water_temperature"
CONF_AMBIENT_TEMPERATURE = "ambient_temperature"
CONF_CURRENT_FLOW = "current_flow"
CONF_FRAMES_RECEIVED = "frames_received"
CONF_CRC_ERRORS = "crc_errors"
CONF_SIGNAL_QUALITY = "signal_quality"

# Unit constants not in esphome.const
UNIT_LITERS_PER_HOUR = "L/h"

DEPENDENCIES = ["multical21"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MULTICAL21_ID): cv.use_id(Multical21Component),
        cv.Optional(CONF_TOTAL_CONSUMPTION): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon="mdi:water",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_MONTH_START_VALUE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER,
            icon="mdi:calendar-month",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_WATER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_WATER_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:thermometer-water",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AMBIENT_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:thermometer",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CURRENT_FLOW): sensor.sensor_schema(
            unit_of_measurement=UNIT_LITERS_PER_HOUR,
            icon="mdi:water-pump",
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FRAMES_RECEIVED): sensor.sensor_schema(
            icon=ICON_COUNTER,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_CRC_ERRORS): sensor.sensor_schema(
            icon="mdi:alert-circle-outline",
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_SIGNAL_QUALITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            icon="mdi:signal",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
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

    if CONF_FRAMES_RECEIVED in config:
        sens = await sensor.new_sensor(config[CONF_FRAMES_RECEIVED])
        cg.add(parent.set_frames_received_sensor(sens))

    if CONF_CRC_ERRORS in config:
        sens = await sensor.new_sensor(config[CONF_CRC_ERRORS])
        cg.add(parent.set_crc_errors_sensor(sens))

    if CONF_SIGNAL_QUALITY in config:
        sens = await sensor.new_sensor(config[CONF_SIGNAL_QUALITY])
        cg.add(parent.set_signal_quality_sensor(sens))
