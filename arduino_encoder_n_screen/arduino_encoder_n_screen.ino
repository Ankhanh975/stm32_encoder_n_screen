#include <Wire.h>
#include <Servo.h>
#if defined(__AVR_ATmega328P__)
#include <avr/interrupt.h>
#endif
#include "CurrentMonitor.h"
#include "DisplayController.h"
#include "EncoderController.h"
#include "VoltageMonitor.h"

// =========================
// Hardware configuration
// =========================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

const int CLK_PIN = 10;
const int DT_PIN  = 11;
const int SW_PIN  = 9;
const int ESC_PIN = 3;
const int CURRENT_SENSOR_PIN = A0;

const int MIN_PWM = 1000;
const int MAX_PWM = 2000;

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =========================
// ESC controller
// =========================

class EscController {
public:
  EscController(int pin, int minPwm, int maxPwm)
    : pin(pin),
      minPwm(minPwm),
      maxPwm(maxPwm) {}

  void begin() {
    esc.attach(pin);
    esc.writeMicroseconds(1000);  // State 1: Minimum throttle
    delay(1000);

    esc.writeMicroseconds(1150);  // State 2: Increase throttle
    delay(1000);

    esc.writeMicroseconds(1000);  // State 3: Return to minimum throttle
    delay(4500);

    currentPwm = minPwm;
    targetPwm = minPwm;
    lastThrottleReductionTime = millis();
  }

  void setEnabled(bool enabled) {
    this->enabled = enabled;
  }

  void setValue(int value) {
    this->value = constrain(value, 0, 100);
  }

  int getValue() const {
    return value;
  }

  bool update(bool batteryLow) {
    targetPwm = enabled
      ? map(value, 0, 100, minPwm, maxPwm)
      : minPwm;

    const unsigned long now = millis();
    const unsigned long rampInterval =
      currentPwm < targetPwm && value > SLOW_RAMP_THRESHOLD
        ? SLOW_RAMP_INTERVAL
        : RAMP_INTERVAL;
    bool throttleReduced = false;
    if (enabled &&
        batteryLow &&
        now - lastThrottleReductionTime >= rampInterval / 2) {
      lastThrottleReductionTime = now;
      if (value > 0) {
        value = max(0, value - LOW_BATTERY_THROTTLE_STEP);
        throttleReduced = true;
      }
    } else if (!batteryLow || !enabled) {
      lastThrottleReductionTime = now;
    }

    targetPwm = enabled
      ? map(value, 0, 100, minPwm, maxPwm)
      : minPwm;

    if (now - lastRampTime < rampInterval) {
      return throttleReduced;
    }

    lastRampTime = now;

    if (currentPwm < targetPwm) {
      currentPwm += RAMP_STEP;
      if (currentPwm > targetPwm) {
        currentPwm = targetPwm;
      }
    } else if (currentPwm > targetPwm) {
      currentPwm -= RAMP_STEP;
      if (currentPwm < targetPwm) {
        currentPwm = targetPwm;
      }
    }

    esc.writeMicroseconds(currentPwm);
    return throttleReduced;
  }

private:
  static const unsigned long RAMP_INTERVAL = 20;
  static const unsigned long SLOW_RAMP_INTERVAL = 40;
  static const int SLOW_RAMP_THRESHOLD = 60;
  static const int RAMP_STEP = 5;
  static const int LOW_BATTERY_THROTTLE_STEP = 2;

  Servo esc;
  int pin;
  int minPwm;
  int maxPwm;
  int value = 14;
  int currentPwm = MIN_PWM;
  int targetPwm = MIN_PWM;
  bool enabled = false;
  unsigned long lastRampTime = 0;
  unsigned long lastThrottleReductionTime = 0;
};

// =========================
// Application
// =========================

EncoderController encoder(CLK_PIN, DT_PIN, SW_PIN);
EscController escController(ESC_PIN, MIN_PWM, MAX_PWM);
DisplayController displayController(display, Wire);
VoltageMonitor voltageMonitor(A1);
CurrentMonitor currentMonitor(CURRENT_SENSOR_PIN);

#if defined(__AVR_ATmega328P__)
ISR(PCINT0_vect) {
  encoder.handleEncoderInterrupt();
}
#endif

const unsigned long DISPLAY_SLEEP_TIMEOUT = 45000;
const int LOW_BATTERY_THROTTLE_THRESHOLD_HUNDREDTHS = 270;

int value = 14;
bool enabled = false;
unsigned long lastInputTime = 0;
bool displayAsleep = false;

int valueStep(int direction) {
  return value < 20 || (direction < 0 && value == 20) ? 1 : 2;
}

void refreshDisplay() {
  const long powerWattsTenths = currentMonitor.getPowerWattsTenths(
    voltageMonitor.getVoltageHundredths() * 3
  );
  displayController.render(
    value,
    enabled,
    powerWattsTenths,
    voltageMonitor.getVoltageHundredths(),
    voltageMonitor.getBatteryBars(),
    currentMonitor.getCurrentMilliamps()
  );
}

void registerInput() {
  lastInputTime = millis();
}

void wakeDisplay() {
  if (displayAsleep) {
    displayController.wake();
    displayAsleep = false;
    refreshDisplay();
  }
}

void setup() {
  value = 14;
  enabled = false;

  escController.begin();

  Wire.begin();
  Wire.setClock(400000);
  Wire.setWireTimeout(25000, true);

  encoder.begin();

  displayController.begin();

  voltageMonitor.begin();
  currentMonitor.begin();
  refreshDisplay();
  pinMode(1, INPUT);
  lastInputTime = millis();
}

void loop() {
  const int rotation = encoder.readRotation();
  bool refreshNeeded = false;

  if (rotation != 0 && !displayAsleep) {
    registerInput();
    int remainingRotation = rotation;
    while (remainingRotation != 0) {
      const int direction = remainingRotation > 0 ? 1 : -1;
      value += direction * valueStep(direction);
      value = constrain(value, 0, 100);
      remainingRotation -= direction;
    }
    escController.setValue(value);
    refreshNeeded = true;
  }

  if (encoder.readButtonPressed()) {
    const bool wasAsleep = displayAsleep;
    registerInput();

    if (wasAsleep) {
      wakeDisplay();
    } else {
      enabled = !enabled;
      if (!enabled) {
        value = 14;
        escController.setValue(value);
      }
      escController.setEnabled(enabled);
      refreshNeeded = true;
    }
  }

  displayController.update();

  if (voltageMonitor.update() && !displayAsleep) {
    refreshNeeded = true;
  }

  if (currentMonitor.update() && !displayAsleep) {
    refreshNeeded = true;
  }

  const bool throttleReduced = escController.update(
    voltageMonitor.getVoltageHundredths() <
      LOW_BATTERY_THROTTLE_THRESHOLD_HUNDREDTHS
  );
  if (throttleReduced) {
    value = escController.getValue();
    if (!displayAsleep) {
      refreshNeeded = true;
    }
  }

  if (refreshNeeded && !displayAsleep) {
    refreshDisplay();
  }

  if (!enabled &&
      !displayAsleep &&
      millis() - lastInputTime >= DISPLAY_SLEEP_TIMEOUT) {
    displayController.sleep();
    displayAsleep = true;
  }
}
