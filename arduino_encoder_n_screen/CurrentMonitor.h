#ifndef CURRENT_MONITOR_H
#define CURRENT_MONITOR_H

#include <Arduino.h>

class CurrentMonitor {
public:
  explicit CurrentMonitor(int pin)
    : pin(pin) {}

  void begin() {
    filteredRaw = analogRead(pin);
    lastReadTime = millis();
  }

  bool update() {
    const unsigned long now = millis();
    if (now - lastReadTime < READ_INTERVAL) {
      return false;
    }

    lastReadTime = now;
    const float sample = analogRead(pin);
    filteredRaw += FILTER_COEFFICIENT * (sample - filteredRaw);
    return true;
  }

  int getRaw() const {
    return static_cast<int>(filteredRaw + 0.5f);
  }

  float getAmps() const {
    const float sensorVoltage =
      (filteredRaw - ZERO_CURRENT_RAW) * ADC_REFERENCE_VOLTS / ADC_MAX_READING;
    const float current = sensorVoltage / ACS712_SENSITIVITY_VOLTS_PER_AMP;
    return current > 0.0f ? current : 0.0f;
  }

  bool isCharging() const {
    return filteredRaw < ZERO_CURRENT_RAW;
  }

  float getPowerWatts(float packVoltage) const {
    return getAmps() * packVoltage;
  }

private:
  static const unsigned long READ_INTERVAL = 150;
  static constexpr float FILTER_COEFFICIENT = 0.3f;
  static constexpr float ZERO_CURRENT_RAW = 512.0f;
  static constexpr float ADC_REFERENCE_VOLTS = 5.0f;
  static constexpr float ADC_MAX_READING = 1023.0f;
  static constexpr float ACS712_SENSITIVITY_VOLTS_PER_AMP = 0.185f;

  int pin;
  float filteredRaw = 0.0f;
  unsigned long lastReadTime = 0;
};

#endif
