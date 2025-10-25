# VL6180X Time-of-Flight Distance Sensor for ESPHome

ESPHome custom component for the VL6180X time-of-flight laser ranging sensor.

## Features

- Accurate distance measurements up to 200mm
- I2C communication with 16-bit register addressing
- Fully implements ST AN4545 initialization sequence
- Single-shot measurement mode
- Easy integration with Home Assistant

## Hardware

**VL6180X Specifications:**
- Range: 0-200mm (0-20cm)
- Interface: I2C (address 0x29)
- Voltage: 2.8V - 3.3V
- Best for close-range, high-accuracy measurements

**Wiring:**
```
VL6180X → ESP32
VCC     → 3.3V
GND     → GND
SDA     → GPIO21 (or any I2C SDA pin)
SCL     → GPIO22 (or any I2C SCL pin)
```

Note: XSHUT and INT pins are optional and not required for basic operation.

## Installation

Add to your ESPHome YAML:

```yaml
external_components:
  - source: github://Nick6464/vl6180x-esphome
    components: [ vl6180x ]

i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 400kHz

sensor:
  - platform: vl6180x
    name: "Distance"
    update_interval: 2s
```

## Configuration Variables

- **name** (*Required*, string): Sensor name
- **update_interval** (*Optional*, time): Update frequency (default: 60s)
- **id** (*Optional*, ID): Manually specify sensor ID
- **address** (*Optional*, int): I2C address (default: 0x29)
- All other standard [sensor options](https://esphome.io/components/sensor/#config-sensor)

## Example Configuration

```yaml
sensor:
  - platform: vl6180x
    name: "Water Level"
    id: water_distance
    update_interval: 1s
    filters:
      - sliding_window_moving_average:
          window_size: 5
          send_every: 1
    on_value:
      then:
        - logger.log:
            format: "Distance: %.0f mm"
            args: ['x']
```

## Usage Notes

- **Range:** VL6180X works best for 5-200mm measurements
- **Ambient Light:** Works in bright ambient light (unlike ultrasonic)
- **Target Surface:** Works well with various materials including water
- **Accuracy:** ~±3mm typical accuracy
- **Update Rate:** Can update up to 10Hz (100ms) in continuous mode

## Troubleshooting

**Sensor not detected:**
- Check I2C wiring (SDA/SCL)
- Verify 3.3V power supply
- Enable I2C scan to confirm device at 0x29

**Readings stuck at 0:**
- Check sensor orientation (lens should face target)
- Ensure target is within 5-200mm range
- Try power cycling the ESP32

**Timeout errors:**
- Reduce update_interval
- Check for I2C bus conflicts
- Verify sensor initialization completed

## Technical Details

This component implements:
- 16-bit I2C register addressing per VL6180X datasheet
- ST AN4545 mandatory private register initialization
- Single-shot ranging mode
- Proper interrupt handling and clearing

## Credits

Developed for the OrchidShelf automated flood irrigation system.

Based on ST Microelectronics VL6180X datasheet and AN4545 application note.

## License

MIT License

