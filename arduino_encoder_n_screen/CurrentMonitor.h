#ifndef CURRENT_MONITOR_H
#define CURRENT_MONITOR_H

#include <Arduino.h>

class CurrentMonitor {
public:
  explicit CurrentMonitor(int pin)
    : pin(pin) {}

  void begin() {
    filteredRawTenths = analogRead(pin) * RAW_SCALE;
    lastReadTime = millis();
  }

  bool update() {
    const unsigned long now = millis();
    if (now - lastReadTime < READ_INTERVAL) {
      return false;
    }

    lastReadTime = now;
    const long sampleRawTenths = analogRead(pin) * RAW_SCALE;
    filteredRawTenths =
      (FILTER_REMAINDER * filteredRawTenths +
       FILTER_NUMERATOR * sampleRawTenths + FILTER_ROUNDING) /
      FILTER_SCALE;
    return true;
  }

  long getCurrentMilliamps() const {
    const long rawDifference = filteredRawTenths - ZERO_CURRENT_TENTHS;
    const long currentMilliamps =
      rawDifference * ADC_MILLIVOLTS * 100L /
      (ADC_MAX_READING * ACS712_SENSITIVITY_MILLIVOLTS_PER_AMP);
    return currentMilliamps;
  }

  long getPowerWattsTenths(int packVoltageHundredths) {
    const long instantaneousPowerTenths =
      getCurrentMilliamps() * packVoltageHundredths / POWER_TENTHS_SCALE;
    if (!powerInitialized) {
      filteredPowerTenths = instantaneousPowerTenths;
      powerInitialized = true;
    } else {
      filteredPowerTenths =
        (FILTER_REMAINDER * filteredPowerTenths +
         FILTER_NUMERATOR * instantaneousPowerTenths + FILTER_ROUNDING) /
        FILTER_SCALE;
    }
    return filteredPowerTenths;
  }

private:
  static const unsigned long READ_INTERVAL = 150;
  static const long FILTER_SCALE = 10;
  static const long FILTER_NUMERATOR = 3;
  static const long FILTER_REMAINDER = FILTER_SCALE - FILTER_NUMERATOR;
  static const long FILTER_ROUNDING = FILTER_SCALE / 2;
  static const long RAW_SCALE = 10;
  static const long ZERO_CURRENT_TENTHS = 512 * RAW_SCALE;
  static const long ADC_MILLIVOLTS = 5000;
  static const long ADC_MAX_READING = 1023;
  static const long ACS712_SENSITIVITY_MILLIVOLTS_PER_AMP = 66;
  static const long POWER_TENTHS_SCALE = 10000;

  int pin;
  long filteredRawTenths = 0;
  long filteredPowerTenths = 0;
  bool powerInitialized = false;
  unsigned long lastReadTime = 0;
};

#endif
