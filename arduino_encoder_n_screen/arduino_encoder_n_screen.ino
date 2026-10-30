#include <Wire.h>
#include <Servo.h>
#include "DisplayController.h"
#include "EncoderController.h"
#include "VoltageMonitor.h"

// =========================
// Hardware configuration
// =========================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

const int CLK_PIN = 8;
const int DT_PIN  = 9;
const int SW_PIN  = 10;
const int ESC_PIN = 3;

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
  }

  void setEnabled(bool enabled) {
    this->enabled = enabled;
  }

  void setValue(int value) {
    this->value = constrain(value, 0, 100);
  }

  void update() {
    targetPwm = enabled
      ? map(value, 0, 100, minPwm, maxPwm)
      : minPwm;

    const unsigned long now = millis();
    if (now - lastRampTime < RAMP_INTERVAL) {
      return;
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
  }

private:
  static const unsigned long RAMP_INTERVAL = 20;
  static const int RAMP_STEP = 5;

  Servo esc;
  int pin;
  int minPwm;
  int maxPwm;
  int value = 14;
  int currentPwm = MIN_PWM;
  int targetPwm = MIN_PWM;
  bool enabled = false;
  unsigned long lastRampTime = 0;
};

// =========================
// Application
// =========================

EncoderController encoder(CLK_PIN, DT_PIN, SW_PIN);
EscController escController(ESC_PIN, MIN_PWM, MAX_PWM);
DisplayController displayController(display, Wire);
VoltageMonitor voltageMonitor(A0);

const unsigned long DISPLAY_SLEEP_TIMEOUT = 45000;

int value = 14;
bool enabled = false;
unsigned long lastInputTime = 0;
bool displayAsleep = false;

int valueStep(int direction) {
    if (value < 16) {
      return 1;
    }
    if (value < 30) {
      return 2;
    }
    if (value < 64) {
      return 4;
    }
    if (value < 80) {
      return 8;
    }
    return 8;
  }

void refreshDisplay() {
  displayController.render(
    value,
    enabled,
    voltageMonitor.getVoltageHundredths(),
    voltageMonitor.getBatteryBars()
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
  refreshDisplay();
  lastInputTime = millis();
}

void loop() {
  displayController.update();

  if (voltageMonitor.update()) {
    if (!displayAsleep) {
      refreshDisplay();
    }
  }

  const int rotation = encoder.readRotation();

  if (rotation != 0 && !displayAsleep) {
    registerInput();
    value += rotation * valueStep(rotation);
    value = constrain(value, 0, 100);
    escController.setValue(value);
    refreshDisplay();
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
      refreshDisplay();
    }
  }

  escController.update();

  if (!enabled &&
      !displayAsleep &&
      millis() - lastInputTime >= DISPLAY_SLEEP_TIMEOUT) {
    displayController.sleep();
    displayAsleep = true;
  }
}
