#ifndef ENCODER_CONTROLLER_H
#define ENCODER_CONTROLLER_H

#include <Arduino.h>

class EncoderController {
public:
  EncoderController(int clkPin, int dtPin, int switchPin)
    : clkPin(clkPin),
      dtPin(dtPin),
      switchPin(switchPin) {}

  void begin() {
    pinMode(clkPin, INPUT_PULLUP);
    pinMode(dtPin, INPUT_PULLUP);
    pinMode(switchPin, INPUT_PULLUP);

    lastEncoderState = readEncoderState();
    lastButtonReading = HIGH;
    stableButtonState = HIGH;
    lastDebounceTime = 0;
    transitionCount = 0;
    lastDirection = 0;
  }

  int readRotation() {
    const uint8_t currentState = readEncoderState();
    int direction = 0;
    int transitionMagnitude = 1;

    if (currentState != lastEncoderState) {
      direction = transitionDirection(lastEncoderState, currentState);

      if (direction != 0) {
        lastDirection = direction;
      } else if ((lastEncoderState ^ currentState) == 0x03 &&
                 lastDirection != 0) {
        // A diagonal jump skips two edges; infer them from the last direction.
        direction = lastDirection;
        transitionMagnitude = 2;
      }

      transitionCount += direction * transitionMagnitude;
      lastEncoderState = currentState;
    }

    if (transitionCount >= 2 || transitionCount <= -2) {
      direction = transitionCount > 0 ? 1 : -1;
      transitionCount -= direction * 2;
      return direction;
    }

    return 0;
  }

  bool readButtonPressed() {
    const bool reading = digitalRead(switchPin);
    bool pressed = false;

    if (reading != lastButtonReading) {
      lastDebounceTime = millis();
    }

    if (millis() - lastDebounceTime > DEBOUNCE_TIME &&
        reading != stableButtonState) {
      stableButtonState = reading;
      pressed = stableButtonState == LOW;
    }

    lastButtonReading = reading;
    return pressed;
  }

private:
  static const unsigned long DEBOUNCE_TIME = 30;

  int clkPin;
  int dtPin;
  int switchPin;
  uint8_t lastEncoderState = 0;
  int transitionCount = 0;
  int lastDirection = 0;
  bool lastButtonReading = HIGH;
  bool stableButtonState = HIGH;
  unsigned long lastDebounceTime = 0;

  uint8_t readEncoderState() const {
    return (digitalRead(clkPin) << 1) | digitalRead(dtPin);
  }

  int transitionDirection(
    uint8_t previousState,
    uint8_t currentState
  ) const {
    if ((previousState == 3 && currentState == 1) ||
        (previousState == 1 && currentState == 0) ||
        (previousState == 0 && currentState == 2) ||
        (previousState == 2 && currentState == 3)) {
      return 1;
    }

    if ((previousState == 1 && currentState == 3) ||
        (previousState == 0 && currentState == 1) ||
        (previousState == 2 && currentState == 0) ||
        (previousState == 3 && currentState == 2)) {
      return -1;
    }

    return 0;
  }
};

#endif
