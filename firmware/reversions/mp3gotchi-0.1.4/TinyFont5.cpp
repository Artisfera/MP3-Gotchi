#include "TinyFont5.h"

namespace {
  // Temporary fallback 3x5 bitmap font, one byte per row, bit 2 is left pixel.
  const uint8_t GLYPH_SPACE[5] = {0,0,0,0,0};
  const uint8_t GLYPH_A[5] = {0b010,0b101,0b111,0b101,0b101};
  const uint8_t GLYPH_B[5] = {0b110,0b101,0b110,0b101,0b110};
  const uint8_t GLYPH_C[5] = {0b011,0b100,0b100,0b100,0b011};
  const uint8_t GLYPH_D[5] = {0b110,0b101,0b101,0b101,0b110};
  const uint8_t GLYPH_E[5] = {0b111,0b100,0b110,0b100,0b111};
  const uint8_t GLYPH_F[5] = {0b111,0b100,0b110,0b100,0b100};
  const uint8_t GLYPH_G[5] = {0b011,0b100,0b101,0b101,0b011};
  const uint8_t GLYPH_H[5] = {0b101,0b101,0b111,0b101,0b101};
  const uint8_t GLYPH_I[5] = {0b111,0b010,0b010,0b010,0b111};
  const uint8_t GLYPH_J[5] = {0b001,0b001,0b001,0b101,0b010};
  const uint8_t GLYPH_K[5] = {0b101,0b101,0b110,0b101,0b101};
  const uint8_t GLYPH_L[5] = {0b100,0b100,0b100,0b100,0b111};
  const uint8_t GLYPH_M[5] = {0b101,0b111,0b111,0b101,0b101};
  const uint8_t GLYPH_N[5] = {0b101,0b111,0b111,0b111,0b101};
  const uint8_t GLYPH_O[5] = {0b010,0b101,0b101,0b101,0b010};
  const uint8_t GLYPH_P[5] = {0b110,0b101,0b110,0b100,0b100};
  const uint8_t GLYPH_Q[5] = {0b010,0b101,0b101,0b111,0b011};
  const uint8_t GLYPH_R[5] = {0b110,0b101,0b110,0b101,0b101};
  const uint8_t GLYPH_S[5] = {0b011,0b100,0b010,0b001,0b110};
  const uint8_t GLYPH_T[5] = {0b111,0b010,0b010,0b010,0b010};
  const uint8_t GLYPH_U[5] = {0b101,0b101,0b101,0b101,0b111};
  const uint8_t GLYPH_V[5] = {0b101,0b101,0b101,0b101,0b010};
  const uint8_t GLYPH_W[5] = {0b101,0b101,0b111,0b111,0b101};
  const uint8_t GLYPH_X[5] = {0b101,0b101,0b010,0b101,0b101};
  const uint8_t GLYPH_Y[5] = {0b101,0b101,0b010,0b010,0b010};
  const uint8_t GLYPH_Z[5] = {0b111,0b001,0b010,0b100,0b111};
  const uint8_t GLYPH_0[5] = {0b111,0b101,0b101,0b101,0b111};
  const uint8_t GLYPH_1[5] = {0b010,0b110,0b010,0b010,0b111};
  const uint8_t GLYPH_2[5] = {0b110,0b001,0b010,0b100,0b111};
  const uint8_t GLYPH_3[5] = {0b110,0b001,0b010,0b001,0b110};
  const uint8_t GLYPH_4[5] = {0b101,0b101,0b111,0b001,0b001};
  const uint8_t GLYPH_5[5] = {0b111,0b100,0b110,0b001,0b110};
  const uint8_t GLYPH_6[5] = {0b011,0b100,0b110,0b101,0b010};
  const uint8_t GLYPH_7[5] = {0b111,0b001,0b010,0b010,0b010};
  const uint8_t GLYPH_8[5] = {0b010,0b101,0b010,0b101,0b010};
  const uint8_t GLYPH_9[5] = {0b010,0b101,0b011,0b001,0b110};
  const uint8_t GLYPH_EXCL[5] = {0b010,0b010,0b010,0b000,0b010};
  const uint8_t GLYPH_DOT[5] = {0b000,0b000,0b000,0b000,0b010};
  const uint8_t GLYPH_COMMA[5] = {0b000,0b000,0b000,0b010,0b100};
  const uint8_t GLYPH_COLON[5] = {0b000,0b010,0b000,0b010,0b000};
  const uint8_t GLYPH_QUESTION[5] = {0b110,0b001,0b010,0b000,0b010};
  const uint8_t GLYPH_SLASH[5] = {0b001,0b001,0b010,0b100,0b100};
  const uint8_t GLYPH_APOSTROPHE[5] = {0b010,0b010,0b000,0b000,0b000};
  const uint8_t GLYPH_MINUS[5] = {0b000,0b000,0b111,0b000,0b000};
}

const uint8_t* TinyFont5::glyph(char c) {
  if (c >= 'a' && c <= 'z') c -= 32;
  switch (c) {
    case 'A': return GLYPH_A; case 'B': return GLYPH_B; case 'C': return GLYPH_C;
    case 'D': return GLYPH_D; case 'E': return GLYPH_E; case 'F': return GLYPH_F;
    case 'G': return GLYPH_G; case 'H': return GLYPH_H; case 'I': return GLYPH_I;
    case 'J': return GLYPH_J; case 'K': return GLYPH_K; case 'L': return GLYPH_L;
    case 'M': return GLYPH_M; case 'N': return GLYPH_N; case 'O': return GLYPH_O;
    case 'P': return GLYPH_P; case 'Q': return GLYPH_Q; case 'R': return GLYPH_R;
    case 'S': return GLYPH_S; case 'T': return GLYPH_T; case 'U': return GLYPH_U;
    case 'V': return GLYPH_V; case 'W': return GLYPH_W; case 'X': return GLYPH_X;
    case 'Y': return GLYPH_Y; case 'Z': return GLYPH_Z;
    case '0': return GLYPH_0; case '1': return GLYPH_1; case '2': return GLYPH_2;
    case '3': return GLYPH_3; case '4': return GLYPH_4; case '5': return GLYPH_5;
    case '6': return GLYPH_6; case '7': return GLYPH_7; case '8': return GLYPH_8;
    case '9': return GLYPH_9; case '!': return GLYPH_EXCL; case '.': return GLYPH_DOT;
    case ',': return GLYPH_COMMA; case ':': return GLYPH_COLON; case '?': return GLYPH_QUESTION;
    case '/': return GLYPH_SLASH; case '\'': return GLYPH_APOSTROPHE; case '-': return GLYPH_MINUS;
    default: return GLYPH_SPACE;
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

void TinyFont5::drawChar(Adafruit_GFX& display, int16_t x, int16_t y, char c, uint16_t color) {
  const uint8_t* rows = glyph(c);
  for (uint8_t yy = 0; yy < 5; yy++) {
    uint8_t row = rows[yy];
    for (uint8_t xx = 0; xx < 3; xx++) {
      if (row & (1 << (2 - xx))) {
        display.drawPixel(x + xx, y + yy, color);
      }
    }
  }
}

void TinyFont5::drawCharClipped(Adafruit_GFX& display, int16_t x, int16_t y, char c, uint16_t color, int16_t clipX, int16_t clipY, uint8_t clipW, uint8_t clipH) {
  const uint8_t* rows = glyph(c);
  int16_t clipRight = clipX + clipW;
  int16_t clipBottom = clipY + clipH;
  for (uint8_t yy = 0; yy < 5; yy++) {
    int16_t py = y + yy;
    if (py < clipY || py >= clipBottom) continue;
    uint8_t row = rows[yy];
    for (uint8_t xx = 0; xx < 3; xx++) {
      int16_t px = x + xx;
      if (px < clipX || px >= clipRight) continue;
      if (row & (1 << (2 - xx))) {
        display.drawPixel(px, py, color);
      }
    }
  }
}

uint16_t TinyFont5::measureText(const char* text) {
  if (!text || !text[0]) return 0;

  uint16_t width = 0;
  const char* p = text;
  while (*p) {
    normalizeByte(p);
    width += charWidth();
  }
  return width > 0 ? width - 1 : 0;
}

void TinyFont5::drawText(Adafruit_GFX& display, int16_t x, int16_t y, const char* text, uint16_t color, uint16_t bgColor, uint8_t maxWidth) {
  display.fillRect(x, y, maxWidth, 6, bgColor);

  int16_t cursorX = x;
  const char* p = text;
  while (*p && cursorX + 3 <= x + maxWidth) {
    char c = normalizeByte(p);
    drawChar(display, cursorX, y, c, color);
    cursorX += 4;
  }
}

void TinyFont5::drawTextClipped(Adafruit_GFX& display, int16_t x, int16_t y, const char* text, uint16_t color, int16_t clipX, int16_t clipY, uint8_t clipW, uint8_t clipH) {
  int16_t cursorX = x;
  const char* p = text;
  while (*p) {
    char c = normalizeByte(p);
    if (cursorX > clipX + clipW) break;
    if (cursorX + 3 >= clipX) {
      drawCharClipped(display, cursorX, y, c, color, clipX, clipY, clipW, clipH);
    }
    cursorX += 4;
  }
}
