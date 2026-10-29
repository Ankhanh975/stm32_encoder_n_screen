#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

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

  void render(
    int value,
    bool enabled,
    int voltageHundredths,
    int batteryPercentage
  ) {
    this->value = value;
    this->enabled = enabled;
    this->voltageHundredths = voltageHundredths;
    this->batteryPercentage = batteryPercentage;
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
  static const uint8_t OLED_ADDRESS = 0x3C;
  static const int16_t STATUS_RECT_X = 0;
  static const int16_t STATUS_RECT_Y = 18;
  static const int16_t STATUS_RECT_HEIGHT = 20;
  static const int16_t BATTERY_X = 2;
  static const int16_t BATTERY_Y = 94;

  Adafruit_SSD1306 &display;
  int value = 14;
  bool enabled = false;
  int voltageHundredths = 0;
  int batteryPercentage = 0;

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

    drawBatteryIndicator();

    display.drawRect(
      STATUS_RECT_X,
      STATUS_RECT_Y,
      display.width(),
      STATUS_RECT_HEIGHT,
      SSD1306_WHITE
    );

    display.display();
  }

  void drawBatteryIndicator() {
    const int clampedPercentage = batteryPercentage < 0
      ? 0
      : (batteryPercentage > 100 ? 100 : batteryPercentage);
    const int filledSegments =
      clampedPercentage == 0 ? 1 : (clampedPercentage + 19) / 20;

    display.drawRect(BATTERY_X, BATTERY_Y, 25, 12, SSD1306_WHITE);
    display.drawRect(BATTERY_X + 25, BATTERY_Y + 3, 3, 6, SSD1306_WHITE);

    for (int segment = 0; segment < filledSegments; ++segment) {
      display.fillRect(
        BATTERY_X + 2 + segment * 4,
        BATTERY_Y + 2,
        3,
        8,
        SSD1306_WHITE
      );
    }
  }
};

#endif
