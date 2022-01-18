from esphome.components import light
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_OUTPUT_ID
from . import axp192_ns, axp192_component, CONF_AXP192_ID

DEPENDENCIES = ['axp192']

axp192_backlight = axp192_ns.class_(
    'axp192_backlight', light.LightOutput, cg.Component)

CONFIG_SCHEMA = cv.All(light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(axp192_backlight),
    cv.Required(CONF_AXP192_ID): cv.use_id(axp192_component)
}).extend(cv.COMPONENT_SCHEMA))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    axp = await cg.get_variable(config[CONF_AXP192_ID])
    cg.add(var.set_axp_parent(axp))
