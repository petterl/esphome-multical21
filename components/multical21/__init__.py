# Multical21 ESPHome Component
# Based on work by:
#   Patrik Thalin - https://github.com/pthalin/esp32-multical21
#   Chester - https://github.com/chester4444/esp-multical21

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi
from esphome.const import CONF_ID

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


def validate_hex_str(length, name):
    """Validate a hex string has the exact expected length and contains only hex chars."""

    def validator(value):
        value = cv.string(value).strip()
        if len(value) != length:
            raise cv.Invalid(
                f"{name} must be exactly {length} hex characters, got {len(value)}"
            )
        try:
            int(value, 16)
        except ValueError:
            raise cv.Invalid(
                f"{name} must contain only hex characters (0-9, a-f, A-F)"
            )
        return value

    return validator


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Multical21Component),
            cv.Required(CONF_GDO0_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_METER_ID): validate_hex_str(
                8, "meter_id"
            ),
            cv.Required(CONF_KEY): validate_hex_str(
                32, "key"
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema(cs_pin_required=True))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    gdo0_pin = await cg.gpio_pin_expression(config[CONF_GDO0_PIN])
    cg.add(var.set_gdo0_pin(gdo0_pin))

    cg.add(var.set_meter_id(config[CONF_METER_ID]))
    cg.add(var.set_key(config[CONF_KEY]))
