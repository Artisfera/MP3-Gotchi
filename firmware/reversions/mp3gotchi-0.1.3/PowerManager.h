#pragma once
#include <Arduino.h>

class PowerManager {
public:
  void begin();
  void markNormalBoot();
  void enterDeepSleep();
};
