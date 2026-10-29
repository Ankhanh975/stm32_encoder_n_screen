#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include "EncoderController.h"
#include "VoltageMonitor.h"

// =========================
// Hardware configuration
// =========================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

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
// Display controller
// =========================

class DisplayController {
public:
  explicit DisplayController(Adafruit_SSD1306 &display)
    : display(display) {}

  bool begin() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
      return false;
    }

    display.setRotation(1);
    display.setTextColor(SSD1306_WHITE);
    showArming();
    return true;
  }

  void render(int value, bool enabled, int voltageHundredths) {
    this->value = value;
    this->enabled = enabled;
    this->voltageHundredths = voltageHundredths;
    draw();
  }

  void sleep() {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }

  void wake() {
    display.ssd1306_command(SSD1306_DISPLAYON);
    draw();
  }

private:
  static const int16_t STATUS_RECT_X = 0;
  static const int16_t STATUS_RECT_Y = 18;
  static const int16_t STATUS_RECT_HEIGHT = 20;

  Adafruit_SSD1306 &display;
  int value = 14;
  bool enabled = false;
  int voltageHundredths = 0;

  void showArming() {
    display.clearDisplay();
    drawCenteredText("ARMING", 1, 33);
    display.display();
  }

  void drawCenteredText(
    const char *text,
    uint8_t textSize,
    int16_t y
  ) {
    int16_t x;
    int16_t textY;
    uint16_t textWidth;
    uint16_t textHeight;

    display.setTextSize(textSize);
    display.getTextBounds(
      text,
      0,
      y,
      &x,
      &textY,
      &textWidth,
      &textHeight
    );

    display.setCursor((display.width() - textWidth) / 2, y);
    display.print(text);
  }

  void draw() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    drawCenteredText(enabled ? "ON" : "OFF", 1, 24);

    char valueText[5];
    if (value == 100) {
      snprintf(valueText, sizeof(valueText), "00");
    } else {
      snprintf(valueText, sizeof(valueText), "%d", value);
    }
    drawCenteredText(valueText, 2, 52);

    char voltageText[7];
    snprintf(
      voltageText,
      sizeof(voltageText),
      "%d.%02d",
      voltageHundredths / 100,
      voltageHundredths % 100
    );
    drawCenteredText(voltageText, 1, 82);

    display.drawRect(
      STATUS_RECT_X,
      STATUS_RECT_Y,
      display.width(),
      STATUS_RECT_HEIGHT,
      SSD1306_WHITE
    );

    display.display();
  }
};

// =========================
// Application
// =========================

EncoderController encoder(CLK_PIN, DT_PIN, SW_PIN);
EscController escController(ESC_PIN, MIN_PWM, MAX_PWM);
DisplayController displayController(display);
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
    voltageMonitor.getVoltageHundredths()
  );
}

void registerInput() {
  lastInputTime = millis();
}

void wakeDisplay() {
  if (displayAsleep) {
    displayController.wake();
    displayAsleep = false;
  }
}

void setup() {
  value = 14;
  enabled = false;

  escController.begin();

  Wire.begin();
  Wire.setClock(400000);

  encoder.begin();

  if (!displayController.begin()) {
    while (1);
  }

  voltageMonitor.begin();
  refreshDisplay();
  lastInputTime = millis();
}

void loop() {
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
