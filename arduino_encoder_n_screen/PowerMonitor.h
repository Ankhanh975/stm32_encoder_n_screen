#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include <Arduino.h>
#include "CurrentMonitor.h"
#include "VoltageMonitor.h"

class PowerMonitor {
public:
  PowerMonitor(
    CurrentMonitor &currentMonitor,
    VoltageMonitor &voltageMonitor
  )
    : currentMonitor(currentMonitor),
      voltageMonitor(voltageMonitor) {}

  void begin() {
    updatePower();
    updateBatteryEstimate();
  }

  void update(bool voltageUpdated, bool currentUpdated) {
    if (!voltageUpdated && !currentUpdated) {
      return;
    }

    updatePower();
    if (voltageUpdated) {
      updateBatteryEstimate();
    }
  }

  long getPowerWattsTenths() const {
    return powerWattsTenths;
  }

  int getBatteryPercentage() const {
    return batteryPercentage;
  }

  int getBatteryBars() const {
    return batteryBars;
  }

  int getBatteryPercentageRatePerHour() const {
    return batteryPercentageRatePerHour;
  }

private:
  static const long BATTERY_BAR_THRESHOLD_OFFSET_MILLIVOLTS = 20;
  static const long BATTERY_CAPACITY_MILLIAMP_HOURS = 2500;

  CurrentMonitor &currentMonitor;
  VoltageMonitor &voltageMonitor;
  long powerWattsTenths = 0;
  int batteryPercentage = 0;
  int batteryBars = 0;
  int batteryPercentageRatePerHour = 0;

  void updatePower() {
    const long currentMilliamps = currentMonitor.getCurrentMilliamps();
    const long currentTenthsAmps = roundMilliampsToTenths(currentMilliamps);
    const long packVoltageTenthsVolts =
      (voltageMonitor.getVoltageHundredths() * 3 + 5) / 10;
    powerWattsTenths = currentTenthsAmps * packVoltageTenthsVolts / 10;
    batteryPercentageRatePerHour =
      calculateBatteryPercentageRatePerHour(currentMilliamps);
  }

  static int calculateBatteryPercentageRatePerHour(long currentMilliamps) {
    const long rateNumerator = -currentMilliamps * 100;
    if (rateNumerator < 0) {
      return -((-rateNumerator + BATTERY_CAPACITY_MILLIAMP_HOURS / 2) /
        BATTERY_CAPACITY_MILLIAMP_HOURS);
    }
    return (rateNumerator + BATTERY_CAPACITY_MILLIAMP_HOURS / 2) /
      BATTERY_CAPACITY_MILLIAMP_HOURS;
  }

  static long roundMilliampsToTenths(long currentMilliamps) {
    if (currentMilliamps < 0) {
      return -((-currentMilliamps + 50) / 100);
    }
    return (currentMilliamps + 50) / 100;
  }

  void updateBatteryEstimate() {
    const long cellVoltageMillivolts =
      voltageMonitor.getVoltageMillivolts();
    batteryPercentage = estimateBatteryPercentage(cellVoltageMillivolts);
    updateBatteryBars(cellVoltageMillivolts);
  }

  void updateBatteryBars(long cellVoltageMillivolts) {
    const int lowerThresholdBars = estimateBarsAtVoltage(
      cellVoltageMillivolts + BATTERY_BAR_THRESHOLD_OFFSET_MILLIVOLTS
    );
    if (lowerThresholdBars < batteryBars) {
      batteryBars = lowerThresholdBars;
      return;
    }

    const int upperThresholdBars = estimateBarsAtVoltage(
      cellVoltageMillivolts - BATTERY_BAR_THRESHOLD_OFFSET_MILLIVOLTS
    );
    if (upperThresholdBars > batteryBars) {
      batteryBars = upperThresholdBars;
    }
  }

  static int estimateBarsAtVoltage(long cellVoltageMillivolts) {
    return estimateBatteryBars(estimateBatteryPercentage(cellVoltageMillivolts));
  }

  static int estimateBatteryBars(int percentage) {
    return percentage == 100 ? 5 : (percentage * 6) / 100;
  }

  static int estimateBatteryPercentage(long cellVoltageMillivolts) {
    static const long voltagePointsMillivolts[] = {
      3000, 3300, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200
    };
    static const int percentagePoints[] = {
      0, 5, 10, 20, 50, 60, 75, 85, 95, 100
    };
    static const int pointCount =
      sizeof(voltagePointsMillivolts) /
      sizeof(voltagePointsMillivolts[0]);

    if (cellVoltageMillivolts <= voltagePointsMillivolts[0]) {
      return percentagePoints[0];
    }
    if (cellVoltageMillivolts >=
        voltagePointsMillivolts[pointCount - 1]) {
      return percentagePoints[pointCount - 1];
    }

    for (int i = 1; i < pointCount; ++i) {
      if (cellVoltageMillivolts <= voltagePointsMillivolts[i]) {
        const long voltageRange =
          voltagePointsMillivolts[i] - voltagePointsMillivolts[i - 1];
        const long voltageAboveLowerPoint =
          cellVoltageMillivolts - voltagePointsMillivolts[i - 1];
        const int percentageRange =
          percentagePoints[i] - percentagePoints[i - 1];
        return percentagePoints[i - 1] +
          (percentageRange * voltageAboveLowerPoint + voltageRange / 2) /
          voltageRange;
      }
    }

    return percentagePoints[pointCount - 1];
  }
};

#endif
