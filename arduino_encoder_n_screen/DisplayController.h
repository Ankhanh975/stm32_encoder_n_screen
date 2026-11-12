#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

class DisplayController {
public:
  DisplayController(Adafruit_SSD1306 &display, TwoWire &wire)
    : display(display),
      wire(wire) {}

  bool begin() {
    initialized = initializeDisplay();
    if (!initialized) {
      return false;
    }

    showArming();
    initialized = checkConnection();
    return initialized;
  }

  void update() {
    const unsigned long now = millis();
    if (!initialized && now - lastRecoveryAttempt >= RECOVERY_INTERVAL) {
      lastRecoveryAttempt = now;
      recover();
    }

    bool warningVisibilityChanged = false;
    if (initialized &&
        !sleeping &&
        lowVoltage &&
        now - lastVoltageBlinkTime >= VOLTAGE_BLINK_INTERVAL) {
      lastVoltageBlinkTime = now;
      voltageVisible = !voltageVisible;
      warningVisibilityChanged = true;
    }

    if (initialized &&
        !sleeping &&
        highCurrent &&
        now - lastCurrentBlinkTime >= CURRENT_BLINK_INTERVAL) {
      lastCurrentBlinkTime = now;
      currentVisible = !currentVisible;
      warningVisibilityChanged = true;
    }

    if (initialized &&
        !sleeping &&
        chargingPower &&
        now - lastChargingBlinkTime >= CURRENT_BLINK_INTERVAL) {
      lastChargingBlinkTime = now;
      batteryBarsVisible = !batteryBarsVisible;
      warningVisibilityChanged = true;
    }

    if (warningVisibilityChanged) {
      drawIfReady();
    }
  }

  void render(
    int value,
    bool enabled,
    long powerWattsTenths,
    int voltageHundredths,
    int batteryBars,
    int batteryPercentage,
    int batteryPercentageRatePerHour,
    long currentMilliamps
  ) {
    const bool shouldBlink = voltageHundredths < VOLTAGE_BLINK_THRESHOLD;
    if (shouldBlink != lowVoltage) {
      lowVoltage = shouldBlink;
      voltageVisible = true;
      lastVoltageBlinkTime = millis();
    }

    this->value = value;
    this->enabled = enabled;
    this->powerWattsTenths = powerWattsTenths;
    this->voltageHundredths = voltageHundredths;
    this->batteryBars = batteryBars;
    this->batteryPercentage = batteryPercentage;
    this->batteryPercentageRatePerHour = batteryPercentageRatePerHour;
    this->currentMilliamps = currentMilliamps;
    const bool shouldBlinkCharging =
      powerWattsTenths <= -CHARGING_POWER_THRESHOLD_TENTHS;
    if (shouldBlinkCharging != chargingPower) {
      chargingPower = shouldBlinkCharging;
      batteryBarsVisible = true;
      lastChargingBlinkTime = millis();
    }
    const long currentMagnitude =
      currentMilliamps < 0 ? -currentMilliamps : currentMilliamps;
    const bool shouldBlinkCurrent =
      currentMagnitude >= CURRENT_BLINK_THRESHOLD_MILLIAMPS;
    if (shouldBlinkCurrent != highCurrent) {
      highCurrent = shouldBlinkCurrent;
      currentVisible = true;
      lastCurrentBlinkTime = millis();
    }
    drawIfReady();
  }

  void sleep() {
    sleeping = true;
    if (!initialized) {
      return;
    }

    display.ssd1306_command(SSD1306_DISPLAYOFF);
    recoverIfNeeded();
  }

  void wake() {
    sleeping = false;
    if (!initialized) {
      return;
    }

    display.ssd1306_command(SSD1306_DISPLAYON);
    if (recoverIfNeeded()) {
      drawIfReady();
    }
  }

private:
  static const uint8_t OLED_ADDRESS = 0x3C;
  static const uint32_t I2C_CLOCK_HZ = 400000;
  static const uint32_t I2C_TIMEOUT_US = 25000;
  static const unsigned long RECOVERY_INTERVAL = 1000;
  static const unsigned long VOLTAGE_BLINK_INTERVAL = 500;
  static const unsigned long CURRENT_BLINK_INTERVAL = 500;
  static const int VOLTAGE_BLINK_THRESHOLD = 300;
  static const long CURRENT_BLINK_THRESHOLD_MILLIAMPS = 20000;
  static const long CHARGING_POWER_THRESHOLD_TENTHS = 100;
  static const int16_t VALUE_BAR_X = 2;
  static const int16_t VALUE_BAR_Y = 21;
  static const int16_t BATTERY_Y = 54;

  Adafruit_SSD1306 &display;
  TwoWire &wire;
  int value = 14;
  bool enabled = false;
  long powerWattsTenths = 0;
  int voltageHundredths = 0;
  int batteryBars = 0;
  int batteryPercentage = 0;
  int batteryPercentageRatePerHour = 0;
  long currentMilliamps = 0;
  bool lowVoltage = false;
  bool highCurrent = false;
  bool chargingPower = false;
  bool voltageVisible = true;
  bool currentVisible = true;
  bool batteryBarsVisible = true;
  bool initialized = false;
  bool sleeping = false;
  unsigned long lastRecoveryAttempt = 0;
  unsigned long lastVoltageBlinkTime = 0;
  unsigned long lastCurrentBlinkTime = 0;
  unsigned long lastChargingBlinkTime = 0;

  bool initializeDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
      return false;
    }

    display.setRotation(1);
    display.setTextColor(SSD1306_WHITE);
    return checkConnection();
  }

  bool checkConnection() {
    wire.beginTransmission(OLED_ADDRESS);
    const bool connected =
      wire.endTransmission() == 0 && !wire.getWireTimeoutFlag();
    wire.clearWireTimeoutFlag();
    return connected;
  }

  bool recoverIfNeeded() {
    if (!checkConnection()) {
      recover();
      return false;
    }
    return true;
  }

  void recover() {
    initialized = false;

    wire.end();
    wire.begin();
    wire.setClock(I2C_CLOCK_HZ);
    wire.setWireTimeout(I2C_TIMEOUT_US, true);

    if (!initializeDisplay()) {
      return;
    }

    if (sleeping) {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
    } else {
      drawFrame();
    }

    initialized = checkConnection();
  }

  void drawIfReady() {
    if (!initialized) {
      return;
    }

    drawFrame();
    recoverIfNeeded();
  }

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

  void drawRightAlignedText(
    const char *text,
    uint8_t textSize,
    int16_t y,
    int16_t rightMargin = 2
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

    display.setCursor(display.width() - rightMargin - textWidth, y);
    display.print(text);
  }

  void drawFrame() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    drawCenteredText("Power", 1, 10);
    drawValue();
    drawValueBar();

    char voltageText[8];
    snprintf(
      voltageText,
      sizeof(voltageText),
      "%d.%02dV",
      voltageHundredths / 100,
      voltageHundredths % 100
    );
    if (voltageVisible) {
      drawRightAlignedText(voltageText, 1, 68);
    }

    char currentText[10];
    const long currentMagnitude =
      currentMilliamps < 0 ? -currentMilliamps : currentMilliamps;
    snprintf(
      currentText,
      sizeof(currentText),
      "%ld.%01ldA",
      currentMagnitude / 1000,
      (currentMagnitude % 1000) / 100
    );
    if (currentVisible) {
      const int16_t currentRightMargin = currentMagnitude < 10000 ? 8 : 2;
      drawRightAlignedText(currentText, 1, 78, currentRightMargin);
    }

    drawBatteryIndicator();
    drawPower();
    drawBatteryPercentage();
    drawBatteryPercentageRate();

    display.display();
  }

  void drawValue() {
    if (!enabled) {
      drawCenteredText("Off", 1, 29);
      return;
    }

    char valueText[12];
    snprintf(valueText, sizeof(valueText), "%d", value);

    int16_t boundsX;
    int16_t boundsY;
    uint16_t valueWidth;
    uint16_t valueHeight;

    display.setTextSize(1);
    display.getTextBounds(
      valueText,
      0,
      0,
      &boundsX,
      &boundsY,
      &valueWidth,
      &valueHeight
    );

    const int16_t percentWidth = 6;
    const int16_t gap = 2;
    const int16_t totalWidth = valueWidth + gap + percentWidth;
    const int16_t x = (display.width() - totalWidth) / 2;

    display.setCursor(x, 29);
    display.print(valueText);
    display.setTextSize(1);
    display.setCursor(x + valueWidth + gap, 29);
    display.print("%");
  }

  void drawPower() {
    char powerText[12];
    const long powerMagnitudeTenths =
      powerWattsTenths < 0 ? -powerWattsTenths : powerWattsTenths;
    if (powerMagnitudeTenths > 1000) {
      snprintf(
        powerText,
        sizeof(powerText),
        "%ldW",
        powerMagnitudeTenths / 10
      );
    } else {
      snprintf(
        powerText,
        sizeof(powerText),
        "%ld.%01ldW",
        powerMagnitudeTenths / 10,
        powerMagnitudeTenths % 10
      );
    }
    drawRightAlignedText(powerText, 1, 88);
  }

  void drawBatteryPercentage() {
    char percentageText[8];
    snprintf(percentageText, sizeof(percentageText), "%d%%", batteryPercentage);
    drawRightAlignedText(percentageText, 1, 98);
  }

  void drawBatteryPercentageRate() {
    char rateText[8];
    if (batteryPercentageRatePerHour > 0) {
      snprintf(rateText, sizeof(rateText), "+%d%%/h", batteryPercentageRatePerHour);
    } else {
      snprintf(rateText, sizeof(rateText), "%d%%/h", batteryPercentageRatePerHour);
    }
    drawRightAlignedText(rateText, 1, 108);
  }

  void drawValueBar() {
    const int clampedValue = value < 0
      ? 0
      : (value > 100 ? 100 : value);
    const int filledWidth = (26 * clampedValue + 50) / 100;

    display.drawRoundRect(
      VALUE_BAR_X,
      VALUE_BAR_Y,
      28,
      4,
      2,
      SSD1306_WHITE
    );

    if (enabled && filledWidth > 0) {
      display.fillRect(
        VALUE_BAR_X + 1,
        VALUE_BAR_Y + 1,
        filledWidth,
        2,
        SSD1306_WHITE
      );
    }
  }

  void drawBatteryIndicator() {
    const int clampedBars = batteryBars < 0
      ? 0
      : (batteryBars > 5 ? 5 : batteryBars);
    const int16_t batteryX = (display.width() - 28) / 2;

    display.drawRect(batteryX, BATTERY_Y + 1, 25, 8, SSD1306_WHITE);
    display.drawRect(batteryX + 25, BATTERY_Y + 3, 3, 4, SSD1306_WHITE);

    for (int segment = 0; segment < clampedBars; ++segment) {
      if (chargingPower &&
          segment == clampedBars - 1 &&
          !batteryBarsVisible) {
        continue;
      }

      display.fillRect(
        batteryX + 2 + segment * 4,
        BATTERY_Y + 2,
        3,
        4,
        SSD1306_WHITE
      );
    }
  }
};

#endif
