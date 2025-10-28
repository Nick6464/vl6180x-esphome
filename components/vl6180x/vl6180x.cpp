#include "vl6180x.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cmath>

namespace esphome {
namespace vl6180x {

static const char *const TAG = "vl6180x";
static const char *const VERSION = "1.0.7";

bool VL6180XSensor::write_reg(uint16_t reg, uint8_t val) {
  uint8_t data[3] = {(uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), val};
  auto result = this->write(data, 3);
  if (result != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Write failed for reg 0x%04X", reg);
    return false;
  }
  delay(1);
  return true;
}

bool VL6180XSensor::read_reg(uint16_t reg, uint8_t *val) {
  uint8_t addr[2] = {(uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF)};
  auto result = this->write_read(addr, 2, val, 1);
  if (result != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Read failed for reg 0x%04X", reg);
    return false;
  }
  return true;
}

float VL6180XSensor::apply_filter(float new_value) {
  // Add new value to the circular buffer
  filter_buffer_.push_back(new_value);
  
  // Keep buffer size limited to filter window
  if (filter_buffer_.size() > filter_window_) {
    filter_buffer_.erase(filter_buffer_.begin());
  }
  
  // Calculate moving average
  float sum = 0.0;
  for (float val : filter_buffer_) {
    sum += val;
  }
  return sum / filter_buffer_.size();
}

void VL6180XSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VL6180X...");
  delay(100);
  yield();
  
  // Check model ID (should be 0xB4)
  uint8_t model_id;
  if (!this->read_reg(0x0000, &model_id)) {
    ESP_LOGE(TAG, "Failed to read model ID - check wiring");
    this->mark_failed();
    return;
  }
  
  if (model_id != 0xB4) {
    ESP_LOGE(TAG, "Wrong model ID! Expected 0xB4, got 0x%02X", model_id);
    this->mark_failed();
    return;
  }
  
  ESP_LOGCONFIG(TAG, "VL6180X detected, model ID: 0x%02X", model_id);
  
  // Check fresh out of reset flag
  uint8_t fresh;
  if (!this->read_reg(0x0016, &fresh)) {
    ESP_LOGE(TAG, "Failed to read fresh reset flag");
    this->mark_failed();
    return;
  }
  
  ESP_LOGCONFIG(TAG, "Fresh out of reset flag: %d", fresh);
  
  // Always load mandatory settings (not just when fresh == 1)
  // This ensures proper initialization even if ESP reboots but sensor doesn't
  ESP_LOGCONFIG(TAG, "Loading mandatory settings...");
  yield();
  
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
  yield();
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
  yield();
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
  yield();
  
  // Clear fresh out of reset flag
  this->write_reg(0x0016, 0x00);
  
  // Allow time for settings to take effect
  delay(10);
  
  ESP_LOGCONFIG(TAG, "Configuring range measurement settings...");
  yield();
  
  // Configure range measurement settings per datasheet recommendations
  // READOUT__AVERAGING_SAMPLE_PERIOD (0x010A) - for improved accuracy
  this->write_reg(0x010A, 0x30);  // Default is 0x30 (48 samples averaged)
  
  // SYSRANGE__VHV_RECALIBRATE (0x002E) - Enable VHV recalibration
  this->write_reg(0x002E, 0x01);
  
  // SYSRANGE__VHV_REPEAT_RATE (0x0031) - Set VHV repeat rate
  this->write_reg(0x0031, 0xFF);
  
  // SYSRANGE__MAX_CONVERGENCE_TIME (0x001C) - max time for measurement
  this->write_reg(0x001C, 0x32);  // 50ms max convergence time
  
  // SYSRANGE__RANGE_CHECK_ENABLES (0x002D) - enable signal and ignore thresholds  
  this->write_reg(0x002D, 0x10);  // Enable SNR check only (bit 4)
  
  // SYSRANGE__MAX_AMBIENT_LEVEL_MULT (0x002C) - ambient light threshold
  this->write_reg(0x002C, 0xFF);  // Maximum tolerance to ambient light
  
  // SYSRANGE__INTERMEASUREMENT_PERIOD (0x001B) - time between ranging operations
  this->write_reg(0x001B, 0x09);  // 100ms period for continuous mode (not used in single-shot)
  
  // SYSTEM__MODE_GPIO1 (0x0011) - Configure GPIO1 as output (not using interrupts for now)
  this->write_reg(0x0011, 0x10);  // GPIO1 = disabled
  
  // SYSTEM__INTERRUPT_CONFIG_GPIO (0x0014) - Configure interrupt behavior
  this->write_reg(0x0014, 0x24);  // Disable all interrupt sources initially
  
  // SYSTEM__GROUPED_PARAMETER_HOLD (0x0017) - Don't hold parameters during updates
  this->write_reg(0x0017, 0x00);
  
  // Allow configuration to settle
  delay(10);
  
  // Clear any stale measurements and interrupts
  ESP_LOGCONFIG(TAG, "Clearing stale data...");
  this->write_reg(0x0015, 0x07);  // Clear all interrupts
  delay(10);
  
  // Read and discard any stale measurement
  uint8_t stale_status;
  if (this->read_reg(0x004F, &stale_status)) {
    if (stale_status & 0x04) {
      uint8_t discard;
      this->read_reg(0x0062, &discard);  // Read and discard stale range
      this->write_reg(0x0015, 0x07);      // Clear interrupt again
      ESP_LOGCONFIG(TAG, "Cleared stale measurement data");
    }
  }
  
  // Do a test read of critical registers
  uint8_t test_val;
  if (this->read_reg(0x001C, &test_val)) {
    ESP_LOGCONFIG(TAG, "SYSRANGE__MAX_CONVERGENCE_TIME: 0x%02X", test_val);
  }
  if (this->read_reg(0x0018, &test_val)) {
    ESP_LOGCONFIG(TAG, "SYSRANGE__START (should be 0x00): 0x%02X", test_val);
  }
  
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
  uint32_t start_time = millis();
  
  // Take multiple samples and average them
  for (uint8_t sample = 0; sample < samples_; sample++) {
    // Timeout check - abort if taking too long (prevent watchdog timeout)
    if (millis() - start_time > 500) {
      ESP_LOGV(TAG, "Update timeout after 500ms, got %d/%d samples", valid_samples, samples_);
      break;
    }
    
    // Yield to prevent watchdog timeout
    yield();
    
    // Clear any previous interrupt flags before starting
    if (!this->write_reg(0x0015, 0x07)) {
      ESP_LOGV(TAG, "Failed to clear interrupt");
      continue;
    }
    
    // Small delay to allow interrupt clear to process
    delay(5);
    
    // Start single-shot range measurement (mode 0x01 = single-shot)
    if (!this->write_reg(0x0018, 0x01)) {
      ESP_LOGV(TAG, "Failed to write start command");
      continue;
    }
    
    // Wait for measurement to complete (typically takes ~30-50ms)
    // Maximum 50 iterations = 100ms timeout per sample
    uint8_t status = 0;
    bool got_measurement = false;
    
    for (int i = 0; i < 50; i++) {
      yield(); // Yield every iteration to be safe
      
      if (!this->read_reg(0x004F, &status)) {
        ESP_LOGW(TAG, "Failed to read status register");
        break; // I2C error, abort this sample
      }
      
      // Log status occasionally at VERBOSE level for debugging
      if (sample == 0 && i == 0) {
        ESP_LOGVV(TAG, "Initial status: 0x%02X", status);
      }
      
      if (status & 0x04) {
        // Measurement ready
        uint8_t error_status;
        if (!this->read_reg(0x004D, &error_status)) {
          ESP_LOGW(TAG, "Failed to read error status");
          break;
        }
        
        uint8_t error_code = (error_status >> 4) & 0x0F;
        
        if (error_code == 0x00) {
          // Valid measurement
          uint8_t range;
          if (this->read_reg(0x0062, &range)) {
            if (range <= 200) {
              sum += range;
              valid_samples++;
              got_measurement = true;
              ESP_LOGV(TAG, "Sample %d: %d mm", sample, range);
            } else {
              ESP_LOGV(TAG, "Sample %d out of range: %d mm", sample, range);
            }
          }
        } else {
          ESP_LOGW(TAG, "Sample %d error code: 0x%X", sample, error_code);
        }
        
        // Clear interrupt
        this->write_reg(0x0015, 0x07);
        break;
      }
      
      delay(2);
    }
    
    if (!got_measurement) {
      ESP_LOGW(TAG, "Sample %d timeout (final status: 0x%02X)", sample, status);
    }
    
    // Brief inter-measurement delay
    if (sample < samples_ - 1) {
      delay(5);
    }
  }
  
  if (valid_samples > 0) {
    float average = (float)sum / valid_samples;
    
    // Apply moving average filter if configured
    float filtered_value = average;
    if (filter_window_ > 1) {
      filtered_value = this->apply_filter(average);
    }
    
    // Apply delta threshold check if configured
    bool should_publish = true;
    if (delta_threshold_ > 0.0 && !std::isnan(last_published_value_)) {
      float delta = abs(filtered_value - last_published_value_);
      if (delta < delta_threshold_) {
        should_publish = false;
      }
    }
    
    if (should_publish) {
      this->publish_state(filtered_value);
      last_published_value_ = filtered_value;
    }
  } else {
    ESP_LOGW(TAG, "No valid measurements");
  }
}

void VL6180XSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "VL6180X Time-of-Flight Distance Sensor:");
  ESP_LOGCONFIG(TAG, "  Library Version: %s", VERSION);
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Distance", this);
  ESP_LOGCONFIG(TAG, "  Samples: %d", samples_);
  ESP_LOGCONFIG(TAG, "  Filter Window: %d", filter_window_);
  ESP_LOGCONFIG(TAG, "  Delta Threshold: %.1f mm", delta_threshold_);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with VL6180X failed!");
  }
}

}  // namespace vl6180x
}  // namespace esphome

