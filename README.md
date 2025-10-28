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
    samples: 3                 # Optional, default 1 (range: 1-5 recommended)
    filter_window: 5           # Optional, default 5 (range: 1-20)
    delta_threshold: 5.0       # Optional, default 0.0 mm (range: 0-50)
```

### Noise Reduction Parameters

**`samples`**: Number of hardware readings to average per update. Each sample takes ~100ms. Recommended maximum is 3-5 to avoid watchdog timeout (500ms limit). Higher values increase measurement time but reduce noise.

**`filter_window`**: Moving average window size. Smooths readings across multiple updates. Larger values provide smoother transitions but slower response to real changes.

**`delta_threshold`**: Minimum change in mm required to publish an update. Prevents publishing small fluctuations. Set to 0 to disable (publishes every reading).

All standard ESPHome sensor options are supported.

## Example

```yaml
sensor:
  - platform: vl6180x
    name: "Water Level"
    update_interval: 2s
    samples: 3           # Average 3 hardware readings per update
    filter_window: 5     # Moving average over 5 updates
    delta_threshold: 5.0 # Only publish if change >= 5mm
```

## Technical Details

- Range: 0-200mm (optimal 5-200mm)
- Interface: I2C (100-400kHz recommended)
- Output: Millimeters
- Measurement time: ~50ms per sample
- Implements ST AN4545 mandatory initialization sequence
- Hardware error status validation per datasheet
- Three-stage noise reduction: hardware averaging, moving average filter, delta threshold
- Automatic stale data clearing on startup
- Watchdog-safe implementation with 500ms timeout protection

## Troubleshooting

### "No valid measurements" error
- Check wiring (VCC, GND, SDA, SCL)
- Verify I2C address (default 0x29) with `i2c: scan: true`
- Ensure sensor is powered with stable 3.3V
- Check pull-up resistors on SDA/SCL lines (4.7kΩ typical)

### Timeout warnings
- Reduce `samples` parameter (recommended: 1-3)
- Each sample takes ~100ms, stay under 5 samples total
- Increase `update_interval` if measurements are too frequent

### Unstable readings
- Increase `filter_window` for smoother values
- Increase `delta_threshold` to filter out noise
- Ensure sensor is mounted firmly (vibration affects readings)
- Keep sensor clean (dust on lens affects accuracy)

## License

MIT

