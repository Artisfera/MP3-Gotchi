#pragma once
#include <Arduino.h>

class MessagePool {
public:
  void begin();
  const char* maybePick();

private:
  struct Message {
    char text[32];
    float weight;
  };

  Message messages[20];
  uint8_t count = 0;
  float totalWeight = 0.0f;

  void addMessage(const char* text, float weight);
  void parseScript(const char* script);
};
