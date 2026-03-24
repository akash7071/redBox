# XOrigin AI Pi Lite - Complete Pin Mapping Reference

## Hardware Overview
- **MCU:** ESP32-S3-WROOM-1
- **Display:** ST7735 128x128 LCD
- **Audio Codec:** ES8311
- **Speaker:** Built-in with amplifier
- **Buttons:** 2x tactile buttons (GPIO1, GPIO42)
- **Battery:** Built-in LiPo with voltage monitoring

---

## Complete Pin Mapping Table

| Function | GPIO Pin | Notes |
|----------|----------|-------|
| **POWER MANAGEMENT** | | |
| Power Control (Keep-Alive) | GPIO10 | **CRITICAL:** Must be set HIGH on boot to stay powered on battery |
| Battery Voltage Monitor | GPIO2 | ADC input, use 12db attenuation, multiply by 2.0 |
| | | |
| **DISPLAY (ST7735 - SPI)** | | |
| SPI Clock (SCK) | GPIO16 | |
| SPI Data (MOSI) | GPIO17 | |
| Chip Select (CS) | GPIO15 | |
| Data/Command (DC) | GPIO7 | |
| Reset | GPIO18 | |
| Backlight PWM | GPIO3 | ⚠️ Strapping pin - works but shows warning |
| | | |
| **AUDIO CODEC (ES8311 - I2C)** | | |
| I2C Data (SDA) | GPIO5 | |
| I2C Clock (SCL) | GPIO4 | |
| I2C Address | 0x18 | ES8311 codec address |
| | | |
| **AUDIO (I2S)** | | |
| I2S Master Clock (MCLK) | GPIO6 | |
| I2S Bit Clock (BCLK) | GPIO14 | |
| I2S Word Select (LRCLK) | GPIO12 | |
| I2S Data Out (DOUT) | GPIO11 | To speaker |
| Speaker Amp Enable | GPIO9 | Turn on before playing audio |
| | | |
| **USER INPUTS** | | |
| Left Button | GPIO1 | Also hardware power button (dual function) |
| Right Button | GPIO42 | Standard GPIO button |
| Hidden Button (under display) | GPIO0 or GPIO46 | Factory/boot button - not accessible when assembled |

---

## Critical Implementation Notes

### 1. Battery Power Management
**MUST DO THIS OR DEVICE SHUTS OFF:**
```yaml
# Keep device powered on battery
switch:
  - platform: gpio
    pin: GPIO10
    id: power_control
    restore_mode: ALWAYS_ON
    internal: true

# Turn on immediately at boot
esphome:
  on_boot:
    priority: 600
    then:
      - switch.turn_on: power_control
```

**How it works:**
- Physical power button (left button) wakes device
- Device boots up
- Firmware MUST set GPIO10 HIGH within ~1 second
- GPIO10 tells PMIC to keep power on
- Release power button - device stays on
- Without GPIO10 HIGH, device shuts off when button released

### 2. Left Button (GPIO1) - Dual Function
- **Hardware function:** Wakes device from power-off
- **Software function:** Readable as normal GPIO input when device is running
- You CAN use it for UI navigation/input
- It's NOT just a power button!

### 3. Display Configuration
```yaml
display:
  - platform: mipi_spi
    model: ST7735
    dimensions:
      width: 128
      height: 128
    rotation: 90  # Adjust as needed for your orientation
    cs_pin: GPIO15
    dc_pin: GPIO7
    reset_pin: GPIO18
```

### 4. Audio Setup
```yaml
# Enable speaker amp before playing audio
switch.turn_on: speaker_amp
# Then play your audio
```

### 5. Battery Monitoring
```yaml
sensor:
  - platform: adc
    pin: GPIO2
    attenuation: 12db  # Use 12db, not 11db (deprecated)
    filters:
      - multiply: 2.0  # Voltage divider compensation
```

---

## Framework Compatibility

| Framework | Display Platform | Audio Support | Notes |
|-----------|-----------------|---------------|-------|
| esp-idf | ✅ mipi_spi | ✅ Full ES8311 | Recommended - all features work |
| arduino | ⚠️ st7735 | ⚠️ Limited | May have WiFi issues, display needs different config |

**Recommendation:** Use `esp-idf` framework for full compatibility.

---

## Common Pitfalls

1. **❌ Forgetting GPIO10** → Device shuts off on battery
2. **❌ Wrong attenuation** → Use `12db` not `11db` for battery ADC
3. **❌ Not enabling speaker amp** → No audio output (GPIO9 must be HIGH)
4. **❌ Using GPIO3 and worrying about warning** → It's fine, warning is informational only
5. **❌ Thinking left button can't be used** → It CAN be read by software!
6. **❌ Using wrong display platform** → `mipi_spi` for esp-idf, `st7735` for arduino

---

## Quick Start Checklist

✅ Set GPIO10 HIGH on boot  
✅ Configure display with mipi_spi platform  
✅ Set up I2C for ES8311 (GPIO5/GPIO4)  
✅ Configure both buttons (GPIO1 and GPIO42)  
✅ Enable speaker amp (GPIO9) before audio playback  
✅ Use 12db attenuation for battery ADC  
✅ Use esp-idf framework  

---

## Example Minimal Working Config

See the full working demo config for a complete example with:
- Multi-screen UI
- Both buttons working
- Audio playback
- Battery monitoring
- Smooth animations

The device should boot, display content, respond to buttons, play audio, and stay powered on battery if all pins are configured correctly.
