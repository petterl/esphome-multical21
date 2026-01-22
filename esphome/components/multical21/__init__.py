import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi
from esphome.const import CONF_ID, CONF_CS_PIN

DEPENDENCIES = ["spi"]
AUTO_LOAD = ["sensor", "text_sensor"]
MULTI_CONF = True

CONF_GDO0_PIN = "gdo0_pin"
CONF_METER_ID = "meter_id"
CONF_KEY = "key"

multical21_ns = cg.esphome_ns.namespace("multical21")
Multical21Component = multical21_ns.class_(
    "Multical21Component", cg.PollingComponent, spi.SPIDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Multical21Component),
            cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_GDO0_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_METER_ID): cv.string,
            cv.Required(CONF_KEY): cv.string,
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema(cs_pin_required=False, default_mode="MODE0", default_data_rate=1e6))
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield spi.register_spi_device(var, config)

    cs_pin = yield cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin(cs_pin))

    gdo0_pin = yield cg.gpio_pin_expression(config[CONF_GDO0_PIN])
    cg.add(var.set_gdo0_pin(gdo0_pin))

    cg.add(var.set_meter_id(config[CONF_METER_ID]))
    cg.add(var.set_key(config[CONF_KEY]))
