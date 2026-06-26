#pragma once
#include <Arduino.h>

enum class HapticPattern : uint8_t {
  Click,
  Track,
  Shake,
  Sleep
};

class Haptics {
public:
  struct Step { uint16_t durationMs; bool on; };

  void begin();
  void play(HapticPattern pattern);
  void update();
  void off();

private:
  const Step* activePattern = nullptr;
  uint8_t patternLength = 0;
  uint8_t stepIndex = 0;
  unsigned long stepStartedMs = 0;
  bool running = false;

  void start(const Step* pattern, uint8_t length);
};
