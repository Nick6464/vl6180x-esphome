#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace vl6180x {

class VL6180XSensor : public sensor::Sensor, public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  void set_samples(uint8_t samples) { samples_ = samples; }

 protected:
  bool write_reg(uint16_t reg, uint8_t val);
  bool read_reg(uint16_t reg, uint8_t *val);
  bool initialized_{false};
  uint8_t samples_{1};
};

}  // namespace vl6180x
}  // namespace esphome

