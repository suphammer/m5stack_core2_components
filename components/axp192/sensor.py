import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_BATTERY_LEVEL, UNIT_PERCENT, ICON_BATTERY
from . import axp192_ns, axp192_component, CONF_AXP192_ID


DEPENDENCIES = ['axp192']
axp192_sensor = axp192_ns.class_('axp192_sensor', sensor.Sensor, cg.Component)

CONFIG_SCHEMA = sensor.sensor_schema(UNIT_PERCENT, ICON_BATTERY, 1).extend({
    cv.GenerateID(): cv.declare_id(axp192_sensor),
    cv.Required(CONF_AXP192_ID): cv.use_id(axp192_component)
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)
    axp = await cg.get_variable(config[CONF_AXP192_ID])
    cg.add(axp.set_batterylevel_sensor(var))
