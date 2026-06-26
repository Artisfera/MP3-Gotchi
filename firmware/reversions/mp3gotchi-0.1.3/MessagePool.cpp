#include "MessagePool.h"
#include "ProjectConfig.h"
#include <stdlib.h>
#include <string.h>

// Edit this block if you want to add short text messages.
// Format:
// "Text", weight;
// "Text without weight";
// If weight is omitted, it is treated as 1.0.
// Keep messages short. The 3x5 font fits about 24 ASCII characters in the bottom strip.
static const char MESSAGE_SCRIPT[] = R"MSG(
"Milego dnia!", 1.0;
"Dobry vibe", 0.8;
"Ziuu dalej", 0.7;
"Monkey mode", 1.2;
"Enjoy!";
"Shake me!", 0.5;
)MSG";

void MessagePool::begin() {
  count = 0;
  totalWeight = 0.0f;
  parseScript(MESSAGE_SCRIPT);
}

const char* MessagePool::maybePick() {
  if (count == 0) return "";
  if (random(0, 100) >= Config::MESSAGE_APPEAR_CHANCE_PERCENT) return "";

  float r = ((float)random(0, 10000) / 10000.0f) * totalWeight;
  float acc = 0.0f;

  for (uint8_t i = 0; i < count; i++) {
    acc += messages[i].weight;
    if (r <= acc) return messages[i].text;
  }

  return messages[count - 1].text;
}

void MessagePool::addMessage(const char* text, float weight) {
  if (count >= Config::MAX_MESSAGES) return;
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

  while (*p && count < Config::MAX_MESSAGES) {
    while (*p && *p != '"') p++;
    if (!*p) break;
    p++;

    char text[32];
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
