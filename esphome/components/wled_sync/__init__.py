import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PORT, CONF_UPDATE_INTERVAL

CODEOWNERS = ["@netmindz"]
AUTO_LOAD = ["socket"]
DEPENDENCIES = ["network"]
MULTI_CONF = True

wled_sync_ns = cg.esphome_ns.namespace("wled_sync")
WLEDSyncComponent = wled_sync_ns.class_("WLEDSyncComponent", cg.Component)

CONF_WLED_SYNC_ID = "wled_sync_id"
CONF_MULTICAST_GROUP = "multicast_group"

# WLED's built-in "UDP Sound Sync" multicast group/port (AudioReactive usermod).
DEFAULT_MULTICAST_GROUP = "239.0.0.1"
DEFAULT_PORT = 11988

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WLEDSyncComponent),
        cv.Optional(
            CONF_MULTICAST_GROUP, default=DEFAULT_MULTICAST_GROUP
        ): cv.ipv4address,
        cv.Optional(CONF_PORT, default=DEFAULT_PORT): cv.port,
        cv.Optional(
            CONF_UPDATE_INTERVAL, default="1s"
        ): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_multicast_group(str(config[CONF_MULTICAST_GROUP])))
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
