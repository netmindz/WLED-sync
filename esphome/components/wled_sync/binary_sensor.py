import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import CONF_WLED_SYNC_ID, WLEDSyncComponent

DEPENDENCIES = ["wled_sync"]

CONF_BEAT = "beat"

ICON_BEAT = "mdi:drum"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
        cv.GenerateID(CONF_WLED_SYNC_ID): cv.use_id(WLEDSyncComponent),
        cv.Optional(CONF_BEAT): binary_sensor.binary_sensor_schema(
            icon=ICON_BEAT,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_WLED_SYNC_ID])

    if beat_config := config.get(CONF_BEAT):
        sens = await binary_sensor.new_binary_sensor(beat_config)
        cg.add(parent.set_beat_binary_sensor(sens))
