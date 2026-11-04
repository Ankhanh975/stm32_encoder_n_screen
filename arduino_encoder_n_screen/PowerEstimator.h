#ifndef POWER_ESTIMATOR_H
#define POWER_ESTIMATOR_H

#include <Arduino.h>

struct EDFEstimate {
  float power;
  float voltage;
  float current;
};

class PowerEstimator {
public:
  void begin() {
    for (int throttle = 0; throttle <= 20; ++throttle) {
      powerTenths[throttle] = static_cast<uint16_t>(
        estimateEDF(throttle).power * 10.0f + 0.5f
      );
    }

    int index = 21;
    for (int throttle = 22; throttle <= 100; throttle += 2) {
      powerTenths[index++] = static_cast<uint16_t>(
        estimateEDF(throttle).power * 10.0f + 0.5f
      );
    }
  }

  float getPowerWatts(int throttle) const {
    int index = -1;
    if (throttle >= 0 && throttle <= 20) {
      index = throttle;
    } else if (throttle >= 22 && throttle <= 100 && throttle % 2 == 0) {
      index = 21 + (throttle - 22) / 2;
    }

    if (index >= 0) {
      return powerTenths[index] / 10.0f;
    }

    return estimateEDF(throttle).power;
  }

private:
  static const int ENTRY_COUNT = 61;
  uint16_t powerTenths[ENTRY_COUNT] = {};

  static EDFEstimate estimateEDF(float throttle) {
    const float maxPower = 310.0f;
    const float batteryVoltage = 12.6f;
    const float internalResistance = 0.09958f;

    throttle = constrain(throttle, 0.0f, 100.0f) / 100.0f;

    float voltage = batteryVoltage;
    float power = 0.0f;
    float current = 0.0f;

    for (int i = 0; i < 20; ++i) {
      const float rpmRatio = throttle * voltage / batteryVoltage;
      power = maxPower * rpmRatio * rpmRatio * rpmRatio;
      current = voltage > 0.0f ? power / voltage : 0.0f;
      voltage = batteryVoltage - current * internalResistance;
    }

    EDFEstimate result;
    result.power = power;
    result.voltage = voltage;
    result.current = current;
    return result;
  }
};

#endif
