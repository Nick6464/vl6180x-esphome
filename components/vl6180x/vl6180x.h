#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"
#include <vector>

namespace esphome {
namespace vl6180x {

class VL6180XSensor : public sensor::Sensor, public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  void set_samples(uint8_t samples) { samples_ = samples; }
  void set_filter_window(uint8_t window) { filter_window_ = window; }
  void set_delta_threshold(float threshold) { delta_threshold_ = threshold; }

 protected:
  bool write_reg(uint16_t reg, uint8_t val);
  bool read_reg(uint16_t reg, uint8_t *val);
  float apply_filter(float new_value);
  
  bool initialized_{false};
  uint8_t samples_{1};
  uint8_t filter_window_{5};
  float delta_threshold_{0.0};
  
  std::vector<float> filter_buffer_;
  float last_published_value_{NAN};
};

}  // namespace vl6180x
}  // namespace esphome

