#pragma once
#include <Arduino.h>
#include "UserConfig.h"

class MessagePool {
public:
  void begin();
  const char* maybePick();

private:
  struct Message {
    char text[64];
    float weight;
  };

  Message messages[UserConfig::MAX_MESSAGES];
  uint8_t count = 0;
  float totalWeight = 0.0f;

  void addMessage(const char* text, float weight);
  void parseScript(const char* script);
};
