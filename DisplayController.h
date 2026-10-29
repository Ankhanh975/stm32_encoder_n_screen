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
  static const int16_t STATUS_BADGE_X = 2;
  static const int16_t STATUS_RECT_Y = 0;
  static const int16_t STATUS_BADGE_WIDTH = 28;
  static const int16_t STATUS_BADGE_HEIGHT = 16;
  static const int16_t VALUE_BAR_X = 2;
  static const int16_t VALUE_BAR_Y = 46;
  static const int16_t BATTERY_X = 2;
  static const int16_t BATTERY_Y = 82;

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

    drawState();
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
    drawCenteredText(voltageText, 1, 68);

    drawBatteryIndicator();

    display.display();
  }

  void drawState() {
    display.drawRoundRect(
      STATUS_BADGE_X,
      STATUS_RECT_Y,
      STATUS_BADGE_WIDTH,
      STATUS_BADGE_HEIGHT,
      4,
      SSD1306_WHITE
    );

    if (enabled) {
      display.fillRoundRect(
        STATUS_BADGE_X,
        STATUS_RECT_Y,
        STATUS_BADGE_WIDTH,
        STATUS_BADGE_HEIGHT,
        4,
        SSD1306_WHITE
      );
      display.setTextColor(SSD1306_BLACK);
    }

    drawCenteredText(enabled ? "ON" : "OFF", 1, 4);
    display.setTextColor(SSD1306_WHITE);
  }

  void drawValue() {
    char valueText[12];
    snprintf(valueText, sizeof(valueText), "%d", value);

    int16_t boundsX;
    int16_t boundsY;
    uint16_t valueWidth;
    uint16_t valueHeight;

    display.setTextSize(2);
    display.getTextBounds(
      valueText,
      0,
      0,
      &boundsX,
      &boundsY,
      &valueWidth,
      &valueHeight
    );
    uint8_t valueTextSize =
      valueWidth + 8 <= display.width() ? 2 : 1;

    display.setTextSize(valueTextSize);
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

    display.setCursor(x, 26);
    display.print(valueText);
    display.setTextSize(1);
    display.setCursor(x + valueWidth + gap, 26);
    display.print("%");
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

    if (filledWidth > 0) {
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
    const int clampedPercentage = batteryPercentage < 0
      ? 0
      : (batteryPercentage > 100 ? 100 : batteryPercentage);
    const int filledSegments = clampedPercentage == 100
      ? 5
      : (clampedPercentage * 6) / 100;

    display.drawRect(BATTERY_X, BATTERY_Y, 25, 10, SSD1306_WHITE);
    display.drawRect(BATTERY_X + 25, BATTERY_Y + 3, 3, 4, SSD1306_WHITE);

    for (int segment = 0; segment < filledSegments; ++segment) {
      display.fillRect(
        BATTERY_X + 2 + segment * 4,
        BATTERY_Y + 2,
        3,
        6,
        SSD1306_WHITE
      );
    }
  }
};

#endif
