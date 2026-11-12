#ifndef VOLTAGE_MONITOR_H
#define VOLTAGE_MONITOR_H

#include <Arduino.h>

class VoltageMonitor {
public:
  explicit VoltageMonitor(int pin)
    : pin(pin) {}

  void begin() {
    filteredVoltageMillivolts = readVoltageMillivolts();
    lastReadTime = millis();
  }

  bool update() {
    const unsigned long now = millis();
    if (now - lastReadTime < READ_INTERVAL) {
      return false;
    }

    lastReadTime = now;
    const long sampleMillivolts = readVoltageMillivolts();
    filteredVoltageMillivolts =
      (4L * filteredVoltageMillivolts + sampleMillivolts + 2L) / 5L;
    return true;
  }

  int getVoltageHundredths() const {
    return (filteredVoltageMillivolts + 5) / 10;
  }

  long getVoltageMillivolts() const {
    return filteredVoltageMillivolts;
  }

private:
  static const unsigned long READ_INTERVAL = 150;
  static const long ADC_TO_CELL_MILLIVOLTS_NUMERATOR = 223800;
  static const long ADC_TO_CELL_MILLIVOLTS_DENOMINATOR = 27621;

  int pin;
  long filteredVoltageMillivolts = 0;
  unsigned long lastReadTime = 0;

  long readVoltageMillivolts() const {
    const long rawReading = analogRead(pin);
    const long scaledReading =
      rawReading * ADC_TO_CELL_MILLIVOLTS_NUMERATOR;
    return (scaledReading + ADC_TO_CELL_MILLIVOLTS_DENOMINATOR / 2) /
           ADC_TO_CELL_MILLIVOLTS_DENOMINATOR;
  }
};

#endif
