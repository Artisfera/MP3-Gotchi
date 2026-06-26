#include "EncoderInput.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include "UserConfig.h"

void EncoderInput::begin() {
  pinMode(Pins::ENCODER_A, INPUT_PULLUP);
  pinMode(Pins::ENCODER_B, INPUT_PULLUP);
  pinMode(Pins::ENCODER_SW, INPUT);
  lastAB = (digitalRead(Pins::ENCODER_A) << 1) | digitalRead(Pins::ENCODER_B);
}

InputEvents EncoderInput::update() {
  InputEvents events;
  events.rotationDetents = readRotationDetents();
  events.clickCount = readButtonClicks(events.longPress);
  return events;
}

int8_t EncoderInput::readRotationDetents() {
  static const int8_t table[16] = {
    0, -1,  1,  0,
    1,  0,  0, -1,
   -1,  0,  0,  1,
    0,  1, -1,  0
  };

  uint8_t ab = (digitalRead(Pins::ENCODER_A) << 1) | digitalRead(Pins::ENCODER_B);
  uint8_t index = (lastAB << 2) | ab;
  lastAB = ab;

  int8_t delta = table[index & 0x0F];
  if (delta == 0) return 0;

  transitionAccum += delta;

  if (transitionAccum >= UserConfig::ENCODER_TRANSITIONS_PER_DETENT) {
    transitionAccum = 0;
    return 1;
  }

  if (transitionAccum <= -UserConfig::ENCODER_TRANSITIONS_PER_DETENT) {
    transitionAccum = 0;
    return -1;
  }

  return 0;
}

uint8_t EncoderInput::readButtonClicks(bool& longPressOut) {
  longPressOut = false;
  uint8_t emittedClicks = 0;
  bool raw = digitalRead(Pins::ENCODER_SW);
  unsigned long now = millis();

  if (raw != lastRawButton) {
    lastRawButton = raw;
    lastDebounceMs = now;
  }

  if ((now - lastDebounceMs) > Config::DEBOUNCE_MS && raw != stableButton) {
    stableButton = raw;

    if (stableButton == LOW) {
      pressedAtMs = now;
      longPressSent = false;
    } else {
      if (!longPressSent) {
        pendingClicks++;
        lastReleaseMs = now;
      }
    }
  }

  if (stableButton == LOW && !longPressSent && (now - pressedAtMs) >= Config::LONG_PRESS_MS) {
    longPressSent = true;
    pendingClicks = 0;
    longPressOut = true;
  }

  if (pendingClicks > 0 && (now - lastReleaseMs) >= Config::MULTI_CLICK_GAP_MS) {
    emittedClicks = pendingClicks;
    pendingClicks = 0;
  }

  return emittedClicks;
}
