import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_UID, CONF_ID, CONF_TYPE
from . import axp192_ns, axp192_component, CONF_AXP192_ID

DEPENDENCIES = ['axp192']

axp192_binary_sensor = axp192_ns.class_(
    'axp192_binary_sensor', binary_sensor.BinarySensor, cg.Component)

monitor_type = axp192_ns.enum('monitor_type')
MONITOR_TYPE = {
    'PLUGGED': monitor_type.MONITOR_PLUGGED,
    'CHARGING': monitor_type.MONITOR_CHARGING,
    'OVERTEMP': monitor_type.MONITOR_OVERTEMP,
    'LOW_BATTERY': monitor_type.MONITOR_LOWBAT,
    'CRITICAL_BATTERY': monitor_type.MONITOR_CRITBAT,
    'CHARGED': monitor_type.MONITOR_CHARGED,
}

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(axp192_binary_sensor),
    cv.Required(CONF_AXP192_ID): cv.use_id(axp192_component),
    cv.Required(CONF_TYPE): cv.enum(MONITOR_TYPE, upper=True, space='_')
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)

    axp = await cg.get_variable(config[CONF_AXP192_ID])
    cg.add(axp.register_monitor(var))
    cg.add(var.set_type(config[CONF_TYPE]))
