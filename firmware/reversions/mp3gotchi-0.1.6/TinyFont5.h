#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>

class TinyFont5 {
public:
  // Temporary fallback 5 px renderer. Replace with generated AsepriteMiniFontData.h after supplying the real font asset.
  static void drawText(Adafruit_GFX& display, int16_t x, int16_t y, const char* text, uint16_t color, uint16_t bgColor, uint8_t maxWidth);
  static void drawTextClipped(Adafruit_GFX& display, int16_t x, int16_t y, const char* text, uint16_t color, int16_t clipX, int16_t clipY, uint8_t clipW, uint8_t clipH);
  static uint16_t measureText(const char* text);
  static uint8_t charWidth() { return 4; }
  static uint8_t charHeight() { return 5; }

private:
  static char normalizeByte(const char*& p);
  static const uint8_t* glyph(char c);
  static void drawChar(Adafruit_GFX& display, int16_t x, int16_t y, char c, uint16_t color);
  static void drawCharClipped(Adafruit_GFX& display, int16_t x, int16_t y, char c, uint16_t color, int16_t clipX, int16_t clipY, uint8_t clipW, uint8_t clipH);
};
