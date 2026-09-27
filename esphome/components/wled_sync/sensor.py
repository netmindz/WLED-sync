import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    ICON_PULSE,
    STATE_CLASS_MEASUREMENT,
    UNIT_HERTZ,
)

ICON_WAVEFORM = "mdi:sine-wave"

from . import CONF_WLED_SYNC_ID, WLEDSyncComponent

DEPENDENCIES = ["wled_sync"]

CONF_VOLUME_SMTH = "volume_smth"
CONF_VOLUME_RAW = "volume_raw"
CONF_FFT_MAGNITUDE = "fft_magnitude"
CONF_FFT_MAJOR_PEAK = "fft_major_peak"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
        cv.GenerateID(CONF_WLED_SYNC_ID): cv.use_id(WLEDSyncComponent),
        cv.Optional(CONF_VOLUME_SMTH): sensor.sensor_schema(
            icon=ICON_PULSE,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_VOLUME_RAW): sensor.sensor_schema(
            icon=ICON_PULSE,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FFT_MAGNITUDE): sensor.sensor_schema(
            icon=ICON_WAVEFORM,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FFT_MAJOR_PEAK): sensor.sensor_schema(
            icon=ICON_WAVEFORM,
            unit_of_measurement=UNIT_HERTZ,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_WLED_SYNC_ID])

    if volume_smth_config := config.get(CONF_VOLUME_SMTH):
        sens = await sensor.new_sensor(volume_smth_config)
        cg.add(parent.set_volume_smth_sensor(sens))
    if volume_raw_config := config.get(CONF_VOLUME_RAW):
        sens = await sensor.new_sensor(volume_raw_config)
        cg.add(parent.set_volume_raw_sensor(sens))
    if fft_magnitude_config := config.get(CONF_FFT_MAGNITUDE):
        sens = await sensor.new_sensor(fft_magnitude_config)
        cg.add(parent.set_fft_magnitude_sensor(sens))
    if fft_major_peak_config := config.get(CONF_FFT_MAJOR_PEAK):
        sens = await sensor.new_sensor(fft_major_peak_config)
        cg.add(parent.set_fft_major_peak_sensor(sens))
