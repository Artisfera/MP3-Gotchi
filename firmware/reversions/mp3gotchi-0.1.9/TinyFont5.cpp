#include "TinyFont5.h"
#include "AsepriteMiniFontData.h"
#include <string.h>

namespace {
  const AsepriteMiniGlyph* findGlyph(char c) {
    for (uint8_t i = 0; i < ASEPRITE_MINI_FONT_GLYPH_COUNT; i++) {
      AsepriteMiniGlyph glyph;
      memcpy_P(&glyph, &ASEPRITE_MINI_FONT[i], sizeof(AsepriteMiniGlyph));
      if (glyph.code == c) return &ASEPRITE_MINI_FONT[i];
    }
    return &ASEPRITE_MINI_FONT[0]; // Space is the first generated glyph.
  }

  void readGlyph(char c, AsepriteMiniGlyph& glyph) {
    if (c >= 'a' && c <= 'z') c -= 32;
    memcpy_P(&glyph, findGlyph(c), sizeof(AsepriteMiniGlyph));
  }
}

char TinyFont5::normalizeByte(const char*& p) {
  unsigned char c = (unsigned char)*p++;
  if (c < 128) return (char)c;

  unsigned char n = (unsigned char)*p++;
  if (c == 0xC3 && n == 0xB3) return 'o';
  if (c == 0xC3 && n == 0x93) return 'O';
  if (c == 0xC4) {
    if (n == 0x85) return 'a'; if (n == 0x84) return 'A';
    if (n == 0x87) return 'c'; if (n == 0x86) return 'C';
    if (n == 0x99) return 'e'; if (n == 0x98) return 'E';
  }
  if (c == 0xC5) {
    if (n == 0x82) return 'l'; if (n == 0x81) return 'L';
    if (n == 0x84) return 'n'; if (n == 0x83) return 'N';
    if (n == 0x9B) return 's'; if (n == 0x9A) return 'S';
    if (n == 0xBA) return 'z'; if (n == 0xB9) return 'Z';
    if (n == 0xBC) return 'z'; if (n == 0xBB) return 'Z';
  }
  return ' ';
}

uint8_t TinyFont5::glyphWidth(char c) {
  AsepriteMiniGlyph glyph;
  readGlyph(c, glyph);
  return glyph.width;
}

uint16_t TinyFont5::measureText(const char* text) {
  if (!text || !text[0]) return 0;

  uint16_t width = 0;
  const char* p = text;
  while (*p) {
    char c = normalizeByte(p);
    width += glyphWidth(c);
  }
  return width;
}

void TinyFont5::drawCharClipped(Adafruit_GFX& display, int16_t x, int16_t y, char c, uint16_t color, int16_t clipX, int16_t clipY, uint8_t clipW, uint8_t clipH) {
  AsepriteMiniGlyph glyph;
  readGlyph(c, glyph);

  int16_t clipRight = clipX + clipW;
  int16_t clipBottom = clipY + clipH;
  for (uint8_t yy = 0; yy < ASEPRITE_MINI_FONT_HEIGHT; yy++) {
    int16_t py = y + yy;
    if (py < clipY || py >= clipBottom) continue;
    uint8_t row = glyph.rows[yy];
    for (uint8_t xx = 0; xx < glyph.width; xx++) {
      int16_t px = x + xx;
      if (px < clipX || px >= clipRight) continue;
      if (row & (1 << (glyph.width - 1 - xx))) {
        display.drawPixel(px, py, color);
      }
    }
  }
}

void TinyFont5::drawText(Adafruit_GFX& display, int16_t x, int16_t y, const char* text, uint16_t color, uint16_t bgColor, uint8_t maxWidth) {
  display.fillRect(x, y, maxWidth, ASEPRITE_MINI_FONT_HEIGHT + 1, bgColor);
  drawTextClipped(display, x, y, text, color, x, y, maxWidth, ASEPRITE_MINI_FONT_HEIGHT + 1);
}

void TinyFont5::drawTextClipped(Adafruit_GFX& display, int16_t x, int16_t y, const char* text, uint16_t color, int16_t clipX, int16_t clipY, uint8_t clipW, uint8_t clipH) {
  int16_t cursorX = x;
  const char* p = text;
  while (*p) {
    char c = normalizeByte(p);
    uint8_t width = glyphWidth(c);
    if (cursorX > clipX + clipW) break;
    if (cursorX + width >= clipX) {
      drawCharClipped(display, cursorX, y, c, color, clipX, clipY, clipW, clipH);
    }
    cursorX += width;
  }
}
