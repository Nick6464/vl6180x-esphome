# VL6180X ESPHome Component

ESPHome custom component for the VL6180X time-of-flight distance sensor.

## Features

- Accurate distance measurements: 5-200mm range
- I2C interface with automatic initialization
- Works in ambient light conditions
- Three-stage noise reduction and debouncing
- Easy Home Assistant integration

## Installation

Add to your ESPHome configuration:

```yaml
external_components:
  - source: github://Nick6464/vl6180x-esphome
    components: [ vl6180x ]

i2c:
  sda: GPIO21
  scl: GPIO22

sensor:
  - platform: vl6180x
    name: "Distance"
    update_interval: 2s
```

## Wiring

```
VL6180X → ESP32
VCC     → 3.3V
GND     → GND
SDA     → GPIO21
SCL     → GPIO22
```

## Configuration

```yaml
sensor:
  - platform: vl6180x
    name: "Distance"           # Required
    update_interval: 2s        # Optional, default 60s
    address: 0x29              # Optional, default 0x29
    samples: 5                 # Optional, default 1 (range: 1-20)
    filter_window: 5           # Optional, default 5 (range: 1-20)
    delta_threshold: 2.0       # Optional, default 0.0 mm (range: 0-50)
```

### Noise Reduction Parameters

**`samples`**: Number of hardware readings to average per update. Higher values increase measurement time but reduce noise.

**`filter_window`**: Moving average window size. Smooths readings across multiple updates. Larger values provide smoother transitions but slower response to real changes.

**`delta_threshold`**: Minimum change in mm required to publish an update. Prevents publishing small fluctuations. Set to 0 to disable.

All standard ESPHome sensor options are supported.

## Example

```yaml
sensor:
  - platform: vl6180x
    name: "Water Level"
    update_interval: 2s
    samples: 5           # Average 5 hardware readings per update
    filter_window: 5     # Moving average over 5 updates
    delta_threshold: 2.0 # Only publish if change >= 2mm
```

## Technical Details

- Range: 0-200mm (optimal 5-200mm)
- Interface: I2C (100-400kHz recommended)
- Output: Millimeters
- Implements ST AN4545 initialization sequence
- Hardware error status validation per datasheet
- Three-stage debouncing: hardware averaging, moving average filter, delta threshold

## License

MIT

