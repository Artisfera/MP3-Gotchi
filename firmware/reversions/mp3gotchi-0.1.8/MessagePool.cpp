#include "MessagePool.h"
#include "ProjectConfig.h"
#include "UserConfig.h"
#include <stdlib.h>
#include <string.h>

void MessagePool::begin() {
  count = 0;
  totalWeight = 0.0f;
  parseScript(UserConfig::MESSAGE_SCRIPT);
}

const char* MessagePool::maybePick() {
  if (count == 0) return "";
  if (random(0, 100) >= UserConfig::MESSAGE_APPEAR_CHANCE_PERCENT) return "";

  float r = ((float)random(0, 10000) / 10000.0f) * totalWeight;
  float acc = 0.0f;

  for (uint8_t i = 0; i < count; i++) {
    acc += messages[i].weight;
    if (r <= acc) return messages[i].text;
  }

  return messages[count - 1].text;
}

void MessagePool::addMessage(const char* text, float weight) {
  if (count >= UserConfig::MAX_MESSAGES) return;
  if (!text || !text[0]) return;
  if (weight <= 0.0f) weight = 1.0f;

  strncpy(messages[count].text, text, sizeof(messages[count].text) - 1);
  messages[count].text[sizeof(messages[count].text) - 1] = '\0';
  messages[count].weight = weight;
  totalWeight += weight;
  count++;
}

void MessagePool::parseScript(const char* script) {
  const char* p = script;

  while (*p && count < UserConfig::MAX_MESSAGES) {
    while (*p && *p != '"') p++;
    if (!*p) break;
    p++;

    char text[64];
    uint8_t len = 0;
    while (*p && *p != '"' && len < sizeof(text) - 1) {
      text[len++] = *p++;
    }
    text[len] = '\0';
    if (*p == '"') p++;

    float weight = 1.0f;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == ',') {
      p++;
      while (*p == ' ' || *p == '\t') p++;
      weight = strtof(p, nullptr);
    }

    while (*p && *p != ';') p++;
    if (*p == ';') p++;

    addMessage(text, weight);
  }
}
