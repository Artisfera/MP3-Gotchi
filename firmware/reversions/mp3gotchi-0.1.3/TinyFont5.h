#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>

class TinyFont5 {
public:
  // Aseprite Mini-compatible 5 px text renderer for the OLED bottom strip.
  static void drawText(Adafruit_GFX& display, int16_t x, int16_t y, const char* text, uint16_t color, uint16_t bgColor, uint8_t maxWidth);
  static uint8_t charWidth() { return 4; }
  static uint8_t charHeight() { return 5; }

private:
  static char normalizeByte(const char*& p);
  static const uint8_t* glyph(char c);
  static void drawChar(Adafruit_GFX& display, int16_t x, int16_t y, char c, uint16_t color);
};
