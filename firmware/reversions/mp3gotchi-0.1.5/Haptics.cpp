#include "Haptics.h"
#include "PinMap.h"

namespace {
  const Haptics::Step CLICK_PATTERN[] = {{25, true}, {1, false}};
  const Haptics::Step TRACK_PATTERN[] = {{35, true}, {45, false}, {35, true}, {1, false}};
  const Haptics::Step SHAKE_PATTERN[] = {{90, true}, {70, false}, {120, true}, {1, false}};
  const Haptics::Step SLEEP_PATTERN[] = {{60, true}, {80, false}, {60, true}, {80, false}, {160, true}, {1, false}};
}

void Haptics::begin() {
  pinMode(Pins::MOTOR_GATE, OUTPUT);
  off();
}

void Haptics::play(HapticPattern pattern) {
  switch (pattern) {
    case HapticPattern::Click: start(CLICK_PATTERN, sizeof(CLICK_PATTERN) / sizeof(CLICK_PATTERN[0])); break;
    case HapticPattern::Track: start(TRACK_PATTERN, sizeof(TRACK_PATTERN) / sizeof(TRACK_PATTERN[0])); break;
    case HapticPattern::Shake: start(SHAKE_PATTERN, sizeof(SHAKE_PATTERN) / sizeof(SHAKE_PATTERN[0])); break;
    case HapticPattern::Sleep: start(SLEEP_PATTERN, sizeof(SLEEP_PATTERN) / sizeof(SLEEP_PATTERN[0])); break;
  }
}

void Haptics::start(const Step* pattern, uint8_t length) {
  activePattern = pattern;
  patternLength = length;
  stepIndex = 0;
  stepStartedMs = millis();
  running = true;
  digitalWrite(Pins::MOTOR_GATE, activePattern[0].on ? HIGH : LOW);
}

void Haptics::update() {
  if (!running || !activePattern) return;
  unsigned long now = millis();
  if ((now - stepStartedMs) < activePattern[stepIndex].durationMs) return;

  stepIndex++;
  if (stepIndex >= patternLength) {
    off();
    running = false;
    activePattern = nullptr;
    return;
  }

  stepStartedMs = now;
  digitalWrite(Pins::MOTOR_GATE, activePattern[stepIndex].on ? HIGH : LOW);
}

void Haptics::off() {
  digitalWrite(Pins::MOTOR_GATE, LOW);
}
