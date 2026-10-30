#ifndef ENCODER_CONTROLLER_H
#define ENCODER_CONTROLLER_H

#include <Arduino.h>

#if defined(__AVR_ATmega328P__)
#include <avr/io.h>
#include <util/atomic.h>
#endif

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

#if defined(__AVR_ATmega328P__)
    if (clkPin >= 8 && clkPin <= 13 &&
        dtPin >= 8 && dtPin <= 13) {
      const uint8_t pinChangeMask =
        _BV(clkPin - 8) | _BV(dtPin - 8);
      PCMSK0 |= pinChangeMask;
      PCIFR |= _BV(PCIF0);
      PCICR |= _BV(PCIE0);
      interruptMode = true;
    }
#endif
  }

  int readRotation() {
#if defined(__AVR_ATmega328P__)
    if (interruptMode) {
      int direction = 0;
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        direction = consumeRotation();
      }
      return direction;
    }
#endif

    updateEncoderState();
    return consumeRotation();
  }

  void handleEncoderInterrupt() {
    updateEncoderState();
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
  volatile uint8_t lastEncoderState = 0;
  volatile int transitionCount = 0;
  volatile int lastDirection = 0;
  bool interruptMode = false;
  bool lastButtonReading = HIGH;
  bool stableButtonState = HIGH;
  unsigned long lastDebounceTime = 0;

  void updateEncoderState() {
    const uint8_t currentState = readEncoderState();
    int direction = 0;
    int transitionMagnitude = 1;

    if (currentState != lastEncoderState) {
      direction = transitionDirection(lastEncoderState, currentState);

      if (direction != 0) {
        lastDirection = direction;
      } else if ((lastEncoderState ^ currentState) == 0x03 &&
                 lastDirection != 0) {
        direction = lastDirection;
        transitionMagnitude = 2;
      }

      transitionCount += direction * transitionMagnitude;
      lastEncoderState = currentState;
    }
  }

  int consumeRotation() {
    const int rotation = transitionCount / 2;
    transitionCount -= rotation * 2;
    return rotation;
  }

  uint8_t readEncoderState() const {
#if defined(__AVR_ATmega328P__)
    if (clkPin >= 8 && clkPin <= 13 &&
        dtPin >= 8 && dtPin <= 13) {
      const uint8_t portState = PINB;
      return (((portState >> (clkPin - 8)) & 1) << 1) |
             ((portState >> (dtPin - 8)) & 1);
    }
#endif
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
