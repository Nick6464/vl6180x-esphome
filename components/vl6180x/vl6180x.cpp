#include "vl6180x.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vl6180x {

static const char *const TAG = "vl6180x";

bool VL6180XSensor::write_reg(uint16_t reg, uint8_t val) {
  uint8_t data[3] = {(uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), val};
  auto result = this->write(data, 3);
  if (result != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Write failed for reg 0x%04X", reg);
    return false;
  }
  return true;
}

bool VL6180XSensor::read_reg(uint16_t reg, uint8_t *val) {
  uint8_t addr[2] = {(uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF)};
  if (this->write(addr, 2) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Write addr failed for reg 0x%04X", reg);
    return false;
  }
  if (this->read(val, 1) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Read failed for reg 0x%04X", reg);
    return false;
  }
  return true;
}

void VL6180XSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VL6180X...");
  delay(100);
  
  // Check model ID (should be 0xB4)
  uint8_t model_id;
  if (!this->read_reg(0x0000, &model_id)) {
    ESP_LOGE(TAG, "Failed to read model ID - check wiring");
    this->mark_failed();
    return;
  }
  
  ESP_LOGI(TAG, "Model ID: 0x%02X", model_id);
  
  if (model_id != 0xB4) {
    ESP_LOGE(TAG, "Wrong model ID! Expected 0xB4, got 0x%02X", model_id);
    this->mark_failed();
    return;
  }
  
  // Check fresh out of reset flag
  uint8_t fresh;
  if (this->read_reg(0x0016, &fresh)) {
    ESP_LOGI(TAG, "Fresh reset status: 0x%02X", fresh);
    
    if (fresh == 1) {
      ESP_LOGI(TAG, "Loading mandatory settings from AN4545...");
      
      // Mandatory private register settings from ST AN4545 application note
      // These must be loaded exactly as specified in the datasheet
      this->write_reg(0x0207, 0x01);
      this->write_reg(0x0208, 0x01);
      this->write_reg(0x0096, 0x00);
      this->write_reg(0x0097, 0xFD);
      this->write_reg(0x00E3, 0x00);
      this->write_reg(0x00E4, 0x04);
      this->write_reg(0x00E5, 0x02);
      this->write_reg(0x00E6, 0x01);
      this->write_reg(0x00E7, 0x03);
      this->write_reg(0x00F5, 0x02);
      this->write_reg(0x00D9, 0x05);
      this->write_reg(0x00DB, 0xCE);
      this->write_reg(0x00DC, 0x03);
      this->write_reg(0x00DD, 0xF8);
      this->write_reg(0x009F, 0x00);
      this->write_reg(0x00A3, 0x3C);
      this->write_reg(0x00B7, 0x00);
      this->write_reg(0x00BB, 0x3C);
      this->write_reg(0x00B2, 0x09);
      this->write_reg(0x00CA, 0x09);
      this->write_reg(0x0198, 0x01);
      this->write_reg(0x01B0, 0x17);
      this->write_reg(0x01AD, 0x00);
      this->write_reg(0x00FF, 0x05);
      this->write_reg(0x0100, 0x05);
      this->write_reg(0x0199, 0x05);
      this->write_reg(0x01A6, 0x1B);
      this->write_reg(0x01AC, 0x3E);
      this->write_reg(0x01A7, 0x1F);
      this->write_reg(0x0030, 0x00);
      
      // Clear fresh out of reset flag
      this->write_reg(0x0016, 0x00);
      ESP_LOGI(TAG, "Settings loaded successfully");
      
      // Allow time for settings to take effect
      delay(10);
    }
  }
  
  // Configure range measurement settings per datasheet recommendations
  // READOUT__AVERAGING_SAMPLE_PERIOD (0x010A) - for improved accuracy
  this->write_reg(0x010A, 0x30);  // Default is 0x30 (48 samples averaged)
  
  // SYSRANGE__MAX_CONVERGENCE_TIME (0x001C) - max time for measurement
  this->write_reg(0x001C, 0x31);  // 49ms max convergence time (recommended for accuracy)
  
  // SYSRANGE__RANGE_CHECK_ENABLES (0x002D) - enable signal and ignore thresholds  
  this->write_reg(0x002D, 0x11);  // Enable early convergence estimate check and SNR check
  
  // SYSRANGE__MAX_AMBIENT_LEVEL_MULT (0x002C) - ambient light threshold
  this->write_reg(0x002C, 0xFF);  // Maximum tolerance to ambient light
  
  // SYSRANGE__INTERMEASUREMENT_PERIOD (0x001B) - time between ranging operations
  this->write_reg(0x001B, 0x09);  // 100ms period for continuous mode (not used in single-shot)
  
  // Allow configuration to settle
  delay(10);
  
  initialized_ = true;
  ESP_LOGCONFIG(TAG, "VL6180X setup complete");
}

void VL6180XSensor::update() {
  if (!initialized_) {
    ESP_LOGW(TAG, "Sensor not initialized");
    return;
  }
  
  uint32_t sum = 0;
  uint8_t valid_samples = 0;
  
  // Take multiple samples and average them
  for (uint8_t sample = 0; sample < samples_; sample++) {
    // Start single-shot range measurement
    if (!this->write_reg(0x0018, 0x01)) {
      ESP_LOGW(TAG, "Failed to start measurement");
      continue;
    }
    
    // Wait for measurement to start (per datasheet, typical 10ms)
    delay(10);
    
    // Poll interrupt status register for range ready (max convergence time ~30ms)
    uint8_t status = 0;
    bool measurement_ready = false;
    
    for (int i = 0; i < 20; i++) {  // Reduced poll count, increased interval
      if (this->read_reg(0x004F, &status)) {
        if (status & 0x04) {
          // Range measurement ready - check error status first
          uint8_t error_status;
          if (!this->read_reg(0x004D, &error_status)) {
            ESP_LOGW(TAG, "Failed to read error status");
            break;
          }
          
          // Check for valid measurement (error code in bits 7:4)
          uint8_t error_code = (error_status >> 4) & 0x0F;
          
          if (error_code == 0x00) {  // No error - valid measurement
            uint8_t range;
            if (this->read_reg(0x0062, &range)) {
              // Additional validation: VL6180X effective range is 0-200mm
              if (range <= 200) {
                sum += range;
                valid_samples++;
                measurement_ready = true;
                ESP_LOGV(TAG, "Sample %d: %d mm (valid)", sample + 1, range);
              } else {
                ESP_LOGV(TAG, "Sample %d: %d mm (out of range)", sample + 1, range);
              }
            }
          } else {
            // Log specific error codes from datasheet
            const char* error_msg = "Unknown";
            switch (error_code) {
              case 0x01: error_msg = "VCSEL Continuity Test"; break;
              case 0x02: error_msg = "VCSEL Watchdog Test"; break;
              case 0x03: error_msg = "VCSEL Watchdog"; break;
              case 0x04: error_msg = "PLL1 Lock"; break;
              case 0x05: error_msg = "PLL2 Lock"; break;
              case 0x06: error_msg = "Early Convergence Estimate"; break;
              case 0x07: error_msg = "Max Convergence"; break;
              case 0x08: error_msg = "No Target Ignore"; break;
              case 0x0B: error_msg = "Max Signal To Noise Ratio"; break;
              case 0x0C: error_msg = "Raw Ranging Algo Underflow"; break;
              case 0x0D: error_msg = "Raw Ranging Algo Overflow"; break;
              case 0x0E: error_msg = "Ranging Algo Underflow"; break;
              case 0x0F: error_msg = "Ranging Algo Overflow"; break;
            }
            ESP_LOGV(TAG, "Sample %d error: 0x%02X (%s)", sample + 1, error_code, error_msg);
          }
          
          // Clear interrupt - must be done after reading results
          this->write_reg(0x0015, 0x07);
          measurement_ready = true;
          break;
        }
      }
      
      // Wait between polls (per datasheet timing requirements)
      delay(5);
    }
    
    if (!measurement_ready) {
      ESP_LOGW(TAG, "Sample %d measurement timeout (status: 0x%02X)", sample + 1, status);
    }
    
    // Inter-measurement delay to allow sensor to stabilize
    if (sample < samples_ - 1) {
      delay(30);
    }
  }
  
  if (valid_samples > 0) {
    float average = (float)sum / valid_samples;
    ESP_LOGD(TAG, "Distance: %.0f mm (averaged from %d/%d samples)", average, valid_samples, samples_);
    this->publish_state(average);
  } else {
    ESP_LOGW(TAG, "No valid measurements obtained");
  }
}

void VL6180XSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "VL6180X Time-of-Flight Distance Sensor:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Distance", this);
  ESP_LOGCONFIG(TAG, "  Samples: %d", samples_);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with VL6180X failed!");
  }
}

}  // namespace vl6180x
}  // namespace esphome

