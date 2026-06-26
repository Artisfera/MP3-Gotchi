#pragma once
#include <Arduino.h>

struct InputEvents {
  int8_t rotationDetents = 0;
  uint8_t clickCount = 0;
  bool longPress = false;
};

class EncoderInput {
public:
  void begin();
  InputEvents update();

private:
  int8_t readRotationDetents();
  uint8_t readButtonClicks(bool& longPressOut);

  uint8_t lastAB = 0;
  int8_t transitionAccum = 0;

  bool lastRawButton = HIGH;
  bool stableButton = HIGH;
  unsigned long lastDebounceMs = 0;
  unsigned long pressedAtMs = 0;
  unsigned long lastReleaseMs = 0;
  uint8_t pendingClicks = 0;
  bool longPressSent = false;
};
