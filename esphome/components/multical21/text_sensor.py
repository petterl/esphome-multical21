import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from . import multical21_ns, Multical21Component

CONF_MULTICAL21_ID = "multical21_id"
CONF_LAST_UPDATE = "last_update"

DEPENDENCIES = ["multical21"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MULTICAL21_ID): cv.use_id(Multical21Component),
        cv.Optional(CONF_LAST_UPDATE): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_MULTICAL21_ID])

    if CONF_LAST_UPDATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_UPDATE])
        cg.add(parent.set_last_update_sensor(sens))
