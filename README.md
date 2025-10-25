# VL6180X ESPHome Component

ESPHome custom component for the VL6180X time-of-flight distance sensor.

## Features

- Accurate distance measurements: 5-200mm range
- I2C interface with automatic initialization
- Works in ambient light conditions
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
```

The `samples` parameter specifies how many readings to average per update to reduce noise. All standard ESPHome sensor options are supported.

## Example

```yaml
sensor:
  - platform: vl6180x
    name: "Water Level"
    update_interval: 1s
    samples: 5  # Hardware averaging to reduce noise
```

## Technical Details

- Range: 0-200mm (optimal 5-200mm)
- Interface: I2C (400kHz)
- Output: Millimeters
- Implements ST AN4545 initialization sequence

## License

MIT

