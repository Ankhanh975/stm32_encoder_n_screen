# Arduino Nano Encoder + OLED ESC Controller

This project is a small Arduino sketch for controlling an electronic speed controller (ESC) from a rotary encoder and a compact OLED display. It is designed for a classic 5 V Arduino Nano (ATmega328P) and uses a 128x32 SSD1306 display to show the current throttle, enable state, and measured voltage.

## Features

- Rotary encoder-based throttle control from 0 to 100
- Button press toggles the ESC on/off
- Smooth ESC ramping to avoid abrupt throttle changes
- OLED status display showing the current throttle, enable state, voltage, and a 1–5 segment battery indicator
- Average voltage per cell for a 3-cell series battery, displayed with two decimal places
- Auto-display sleep after inactivity
- Built-in ESC startup sequence for safe arming

## Hardware

The sketch expects the following hardware:

- Classic 5 V Arduino Nano (ATmega328P)
- SSD1306 128x32 I2C OLED display
- Rotary encoder with CLK, DT, and SW pins
- ESC connected to a PWM signal pin
- A 4:1 voltage divider with its output connected to A0 and its low side connected to common ground
- Common 5V/GND power distribution

## Pin mapping

| Function | Pin |
| --- | --- |
| Encoder CLK | 8 |
| Encoder DT | 9 |
| Encoder SW | 10 |
| ESC signal | 3 |
| Voltage divider output | A0 |
| OLED SDA | SDA |
| OLED SCL | SCL |
| OLED GND | GND |
| OLED VCC | 3.3V or 5V depending on module |

Note: The sketch converts the raw Nano ADC reading directly to calibrated hundredths of a volt per cell, without rounding the pack voltage at intermediate steps. The conversion factor includes the 10-bit ADC, 5 V reference, 4:1 divider, 9.0 V-to-11.19 V calibration, and division across three cells. This is the average voltage of a 3-cell series battery, not an individual cell measurement; measuring each cell separately requires cell-tap connections. The ADC step is about 0.008 V per cell, so two displayed decimals do not imply 0.01 V measurement accuracy. It assumes the source and Nano share a ground. Keep A0 at or below 5 V; the divider must keep the pin within that limit. If hardware or calibration changes, update the conversion constants in `VoltageMonitor.h`.

## Libraries

Install these libraries in the Arduino IDE or PlatformIO environment:

- `Wire`
- `Adafruit_GFX`
- `Adafruit_SSD1306`
- `Servo`

## Operation

1. Upload the sketch to the Arduino Nano.
2. Power the ESC and display.
3. Rotate the encoder to adjust the throttle value.
4. Press the encoder button to arm/disarm the ESC.
5. The OLED displays the power state (`ON`/`OFF`), the active value, measured voltage per cell, and a battery indicator with one to five filled segments based on estimated charge.

## Notes

- The ESC startup routine intentionally performs a safe arming sequence before normal operation.
- The throttle is constrained to a range of 0 to 100 and is only sent to the ESC when enabled.
- Display sleep is enabled after a timeout to reduce burn-in and save power.
- The code uses `Servo` for the ESC signal and expects the ESC to accept standard PWM input signals.

## File structure

- `stm32_encoder_n_screen.ino` — main application sketch
- `DisplayController.h` — OLED display initialization and rendering
- `EncoderController.h` — rotary encoder and button input logic
- `VoltageMonitor.h` — voltage measurement and battery percentage estimation

## License

This project is provided as-is for personal and experimental use. If you plan to reuse or extend it, please keep the original attribution and follow the licensing terms of the included libraries.
