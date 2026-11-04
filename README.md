# EDF Fan Controller

This project uses a rotary encoder and an OLED screen to control an electronic
speed controller (ESC) connected to an electric ducted fan (EDF). The
microcontroller sends throttle commands to the ESC, reads the battery voltage,
and displays the controller state.

## Features

- Adjust throttle from 0 to 100 with the rotary encoder.
- Press the encoder button to enable or disable the ESC output.
- Ramp throttle changes gradually rather than applying them immediately.
- Show the enabled state, throttle, estimated battery voltage per cell, and a
  battery-level indicator on the OLED.
- Blink the voltage readout below 3.00 V per cell.
- Turn off the display after 45 seconds of inactivity while the ESC is disabled.
- Retry OLED initialization after an I2C/display connection problem.

## Hardware

The firmware is currently written for a classic 5 V Arduino Nano with an
ATmega328P. It also expects:

- An EDF unit driven by a compatible ESC.
- A rotary encoder with CLK, DT, and push-button (SW) connections.
- A 128x32 SSD1306 I2C OLED display.
- A battery voltage divider connected to analog input A0. The current voltage
  conversion is calibrated for a 4:1 divider and a 3-cell battery pack.
- A suitable power source and common ground for the Arduino, ESC signal, and
  voltage divider.

Although the repository name mentions STM32, this sketch uses ATmega328P
registers and classic Nano pin assignments for encoder interrupts. An STM32
board will need firmware and pin-mapping changes before use.

## Pin connections

| Component | Arduino Nano |
| --- | --- |
| Encoder CLK | D8 |
| Encoder DT | D9 |
| Encoder push button (SW) | D10 |
| ESC signal input | D3 |
| Voltage-divider output | A0 |
| OLED SDA | SDA |
| OLED SCL | SCL |
| OLED power | 3.3 V or 5 V, according to the display module |
| Ground | GND (shared ground) |

Connect the ESC signal ground and voltage-divider ground to the Arduino ground.
The divider must keep A0 at or below 5 V. Do not connect the battery voltage
directly to A0.

The displayed cell voltage is an average calculated from the total voltage of a
3-cell series pack; it does not measure individual cells. Its conversion
constants are in `arduino_encoder_n_screen/VoltageMonitor.h`. Recalibrate them
if the board, reference voltage, divider, or battery configuration changes.
The two displayed decimal places do not represent 0.01 V measurement accuracy.

## Firmware setup

1. Install the Arduino IDE and select a classic Arduino Nano / ATmega328P board.
2. Install the `Adafruit GFX Library` and `Adafruit SSD1306` libraries.
   `Wire` and `Servo` are also used by the sketch and are commonly included with
   the Arduino AVR board package.
3. Open `arduino_encoder_n_screen/arduino_encoder_n_screen.ino`.
4. Verify the wiring and voltage-divider calibration, then compile and upload.

## Operation

1. Secure the EDF and clear the fan's inlet and exhaust before powering the
   system. Keep people and loose objects away from the fan.
2. Power the system. The firmware runs an ESC arming sequence, sending minimum
   throttle, a brief 1150 µs signal, then minimum throttle again.
3. Rotate the encoder to set the throttle value.
4. Press the encoder button to enable the ESC. Press it again to disable it.
5. The OLED shows the power state, throttle, estimated voltage per cell, and
   battery indicator.

The throttle signal uses 1000–2000 µs pulses. Confirm that these signal limits
and the arming sequence are appropriate for the specific ESC before connecting
the EDF. Disabling the controller commands minimum throttle; it is not a
replacement for disconnecting battery power.

## Project files

- `arduino_encoder_n_screen/arduino_encoder_n_screen.ino` — application,
  ESC control, and hardware configuration.
- `arduino_encoder_n_screen/DisplayController.h` — OLED rendering and I2C
  recovery.
- `arduino_encoder_n_screen/EncoderController.h` — encoder and button input.
- `arduino_encoder_n_screen/VoltageMonitor.h` — voltage conversion, filtering,
  and battery-level estimation.
