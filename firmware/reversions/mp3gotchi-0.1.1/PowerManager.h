#pragma once
#include <Arduino.h>

class PowerManager {
public:
  void begin();
  bool wokeFromLockedSleep() const;
  void markNormalBoot();
  void enterDeepSleep();
};
