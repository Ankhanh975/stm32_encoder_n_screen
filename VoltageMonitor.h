#ifndef VOLTAGE_MONITOR_H
#define VOLTAGE_MONITOR_H

#include <Arduino.h>

class VoltageMonitor {
public:
  explicit VoltageMonitor(int pin)
    : pin(pin) {}

  void begin() {
    filteredVoltageMillivolts = readVoltageMillivolts();
    batteryPercentage = estimateBatteryPercentage(filteredVoltageMillivolts);
    batteryBars = estimateBatteryBars(batteryPercentage);
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
    updateBatteryEstimate();
    return true;
  }

  int getVoltageHundredths() const {
    return (filteredVoltageMillivolts + 5) / 10;
  }

  int getBatteryPercentage() const {
    return batteryPercentage;
  }

  int getBatteryBars() const {
    return batteryBars;
  }

  static int estimateBatteryBars(int percentage) {
    return percentage == 100 ? 5 : (percentage * 6) / 100;
  }

  // Approximate resting-voltage curve for a typical 4.2 V Li-ion cell.
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

private:
  static const unsigned long READ_INTERVAL = 150;
  static const long BATTERY_BAR_RECOVERY_MILLIVOLTS = 40;
  static const long ADC_TO_CELL_MILLIVOLTS_NUMERATOR = 223800;
  static const long ADC_TO_CELL_MILLIVOLTS_DENOMINATOR = 27621;

  int pin;
  long filteredVoltageMillivolts = 0;
  int batteryPercentage = 0;
  int batteryBars = 0;
  unsigned long lastReadTime = 0;

  void updateBatteryEstimate() {
    batteryPercentage = estimateBatteryPercentage(filteredVoltageMillivolts);
    const int estimatedBars = estimateBatteryBars(batteryPercentage);
    if (estimatedBars < batteryBars) {
      batteryBars = estimatedBars;
      return;
    }

    const int recoveredPercentage = estimateBatteryPercentage(
      filteredVoltageMillivolts - BATTERY_BAR_RECOVERY_MILLIVOLTS
    );
    const int recoveredBars = estimateBatteryBars(recoveredPercentage);
    if (recoveredBars > batteryBars) {
      batteryBars = recoveredBars;
    }
  }

  long readVoltageMillivolts() const {
    const long rawReading = analogRead(pin);
    const long scaledReading =
      rawReading * ADC_TO_CELL_MILLIVOLTS_NUMERATOR;
    return (scaledReading + ADC_TO_CELL_MILLIVOLTS_DENOMINATOR / 2) /
           ADC_TO_CELL_MILLIVOLTS_DENOMINATOR;
  }
};

#endif
