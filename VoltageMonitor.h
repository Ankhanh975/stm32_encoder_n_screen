#ifndef VOLTAGE_MONITOR_H
#define VOLTAGE_MONITOR_H

#include <Arduino.h>

class VoltageMonitor {
public:
  explicit VoltageMonitor(int pin)
    : pin(pin) {}

  void begin() {
    voltageHundredths = readVoltageHundredths();
    lastReadTime = millis();
  }

  bool update() {
    const unsigned long now = millis();
    if (now - lastReadTime < READ_INTERVAL) {
      return false;
    }

    lastReadTime = now;
    voltageHundredths = readVoltageHundredths();
    return true;
  }

  int getVoltageHundredths() const {
    return voltageHundredths;
  }

  // Approximate resting-voltage curve for a typical 4.2 V Li-ion cell.
  int estimateBatteryPercentage(float cellVoltage) const {
    static const float voltagePoints[] = {
      3.00, 3.30, 3.50, 3.60, 3.70, 3.80, 3.90, 4.00, 4.10, 4.20
    };
    static const int percentagePoints[] = {
      0, 5, 10, 20, 40, 60, 75, 85, 95, 100
    };
    static const int pointCount =
      sizeof(voltagePoints) / sizeof(voltagePoints[0]);

    if (cellVoltage <= voltagePoints[0]) {
      return percentagePoints[0];
    }
    if (cellVoltage >= voltagePoints[pointCount - 1]) {
      return percentagePoints[pointCount - 1];
    }

    for (int i = 1; i < pointCount; ++i) {
      if (cellVoltage <= voltagePoints[i]) {
        const float fraction =
          (cellVoltage - voltagePoints[i - 1]) /
          (voltagePoints[i] - voltagePoints[i - 1]);
        return percentagePoints[i - 1] +
          fraction * (percentagePoints[i] - percentagePoints[i - 1]);
      }
    }

    return percentagePoints[pointCount - 1];
  }

private:
  static const unsigned long READ_INTERVAL = 150;
  static const long ADC_TO_CELL_HUNDREDTHS_NUMERATOR = 22380;
  static const long ADC_TO_CELL_HUNDREDTHS_DENOMINATOR = 27621;

  int pin;
  int voltageHundredths = 0;
  unsigned long lastReadTime = 0;

  int readVoltageHundredths() const {
    const long rawReading = analogRead(pin);
    const long scaledReading =
      rawReading * ADC_TO_CELL_HUNDREDTHS_NUMERATOR;
    return (scaledReading + ADC_TO_CELL_HUNDREDTHS_DENOMINATOR / 2) /
           ADC_TO_CELL_HUNDREDTHS_DENOMINATOR;
  }
};

#endif
