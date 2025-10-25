import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    DEVICE_CLASS_DISTANCE,
    STATE_CLASS_MEASUREMENT,
    UNIT_MILLIMETER,
)

DEPENDENCIES = ["i2c"]

CONF_SAMPLES = "samples"
CONF_FILTER_WINDOW = "filter_window"
CONF_DELTA_THRESHOLD = "delta_threshold"

vl6180x_ns = cg.esphome_ns.namespace("vl6180x")
VL6180XSensor = vl6180x_ns.class_(
    "VL6180XSensor", sensor.Sensor, cg.PollingComponent, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        VL6180XSensor,
        unit_of_measurement=UNIT_MILLIMETER,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_DISTANCE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(): cv.declare_id(VL6180XSensor),
            cv.Optional(CONF_SAMPLES, default=1): cv.int_range(min=1, max=20),
            cv.Optional(CONF_FILTER_WINDOW, default=5): cv.int_range(min=1, max=20),
            cv.Optional(CONF_DELTA_THRESHOLD, default=0.0): cv.float_range(
                min=0.0, max=50.0
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x29))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_samples(config[CONF_SAMPLES]))
    cg.add(var.set_filter_window(config[CONF_FILTER_WINDOW]))
    cg.add(var.set_delta_threshold(config[CONF_DELTA_THRESHOLD]))
