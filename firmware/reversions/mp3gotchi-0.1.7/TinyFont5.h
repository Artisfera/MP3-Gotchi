#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "AsepriteMiniFontData.h"

class TinyFont5 {
public:
  // Real Aseprite Mini 5 px bitmap renderer generated from the supplied OTF asset.
  static void drawText(Adafruit_GFX& display, int16_t x, int16_t y, const char* text, uint16_t color, uint16_t bgColor, uint8_t maxWidth);
  static void drawTextClipped(Adafruit_GFX& display, int16_t x, int16_t y, const char* text, uint16_t color, int16_t clipX, int16_t clipY, uint8_t clipW, uint8_t clipH);
  static uint16_t measureText(const char* text);
  static uint8_t charHeight() { return ASEPRITE_MINI_FONT_HEIGHT; }

private:
  static char normalizeByte(const char*& p);
  static uint8_t glyphWidth(char c);
  static void drawCharClipped(Adafruit_GFX& display, int16_t x, int16_t y, char c, uint16_t color, int16_t clipX, int16_t clipY, uint8_t clipW, uint8_t clipH);
};
