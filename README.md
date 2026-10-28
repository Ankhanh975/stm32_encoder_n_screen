# STM32 Encoder + OLED ESC Controller

This project is a small Arduino sketch for controlling an electronic speed controller (ESC) from a rotary encoder and a compact OLED display. It is designed for an STM32-based Arduino-compatible board and uses a 128x32 SSD1306 display to show the current throttle and enable state.

## Features

- Rotary encoder-based throttle control from 0 to 100
- Button press toggles the ESC on/off
- Smooth ESC ramping to avoid abrupt throttle changes
- OLED status display showing the current throttle and enable state
- Auto-display sleep after inactivity
- Built-in ESC startup sequence for safe arming

## Hardware

The sketch expects the following hardware:

- STM32 board running the Arduino core
- SSD1306 128x32 I2C OLED display
- Rotary encoder with CLK, DT, and SW pins
- ESC connected to a PWM signal pin
- Common 5V/GND power distribution

## Pin mapping

| Function | Pin |
| --- | --- |
| Encoder CLK | 8 |
| Encoder DT | 9 |
| Encoder SW | 10 |
| ESC signal | 3 |
| OLED SDA | SDA |
| OLED SCL | SCL |
| OLED GND | GND |
| OLED VCC | 3.3V or 5V depending on module |

Note: The sketch uses the default Arduino I2C pins for the OLED and hardware digital pins for the encoder and ESC, so confirm the STM32 board variant matches the selected pin numbers.

## Libraries

Install these libraries in the Arduino IDE or PlatformIO environment:

- `Wire`
- `Adafruit_GFX`
- `Adafruit_SSD1306`
- `Servo`

## Operation

1. Upload the sketch to the STM32 board.
2. Power the ESC and display.
3. Rotate the encoder to adjust the throttle value.
4. Press the encoder button to arm/disarm the ESC.
5. The OLED displays the power state (`ON`/`OFF`) and the active value.

## Notes

- The ESC startup routine intentionally performs a safe arming sequence before normal operation.
- The throttle is constrained to a range of 0 to 100 and is only sent to the ESC when enabled.
- Display sleep is enabled after a timeout to reduce burn-in and save power.
- The code uses `Servo` for the ESC signal and expects the ESC to accept standard PWM input signals.

## File structure

- `stm32_encoder_n_screen.ino` — complete Arduino sketch

## License

This project is provided as-is for personal and experimental use. If you plan to reuse or extend it, please keep the original attribution and follow the licensing terms of the included libraries.
