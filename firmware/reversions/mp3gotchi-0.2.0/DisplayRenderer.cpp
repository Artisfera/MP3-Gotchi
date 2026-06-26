#include "DisplayRenderer.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include "UserConfig.h"
#include "TinyFont5.h"
#include <esp_system.h>
#include <string.h>

namespace {
  constexpr uint16_t COLOR_BLACK = 0x0000;
  constexpr uint16_t COLOR_WHITE = 0xFFFF;
  constexpr uint16_t COLOR_DIM_WHITE = 0xC618;
  constexpr uint8_t DISPLAY_W = 96;
  constexpr uint8_t DISPLAY_H = 64;

  constexpr uint8_t TEXT_X = 0;
  constexpr uint8_t TEXT_Y = 58;
  constexpr uint8_t TEXT_W = 96;
  constexpr uint8_t TEXT_H = 6;
}

namespace {
  uint8_t scaleByte(uint8_t value, uint8_t percent) {
    if (percent == 0) return 0;
    uint16_t scaled = ((uint16_t)value * percent) / 100;
    if (scaled == 0) scaled = 1;
    if (scaled > 255) scaled = 255;
    return (uint8_t)scaled;
  }
}

DisplayRenderer::DisplayRenderer()
  : display(Pins::OLED_CS, Pins::OLED_DC, Pins::OLED_MOSI, Pins::OLED_SCK, Pins::OLED_RST) {}

void DisplayRenderer::begin() {
  display.begin();
  display.setRotation(0);
  buildBaseCache();
  display.fillScreen(COLOR_BLACK);
  randomSeed((uint32_t)esp_random());
  nextBlinkMs = millis() + random(Config::BLINK_EVERY_MIN_MS, Config::BLINK_EVERY_MAX_MS);
  resetCache();
}

void DisplayRenderer::setBrightness(uint8_t percent) {
  if (percent > 100) percent = 100;
  brightnessPercent = percent;

  uint8_t hardwarePercent = percent == 0 ? 0 : 100;
  uint8_t master = scaleByte(0x06, hardwarePercent);
  uint8_t contrastA = scaleByte(0x91, hardwarePercent);
  uint8_t contrastB = scaleByte(0x50, hardwarePercent);
  uint8_t contrastC = scaleByte(0x7D, hardwarePercent);

  display.sendCommand(SSD1331_CMD_MASTERCURRENT);
  display.sendCommand(master);
  display.sendCommand(SSD1331_CMD_CONTRASTA);
  display.sendCommand(contrastA);
  display.sendCommand(SSD1331_CMD_CONTRASTB);
  display.sendCommand(contrastB);
  display.sendCommand(SSD1331_CMD_CONTRASTC);
  display.sendCommand(contrastC);
  display.sendCommand(percent == 0 ? SSD1331_CMD_DISPLAYOFF : SSD1331_CMD_DISPLAYON);
  resetCache();
}

unsigned long DisplayRenderer::bottomTextPassDurationMs(const char* text) const {
  if (!text || !text[0]) return 0;

  uint16_t width = TinyFont5::measureText(text);
  uint16_t travelPx = width + TEXT_W + 2;
  int16_t step = UserConfig::TEXT_SCROLL_STEP_PX;
  if (step <= 0) step = 1;
  return ((unsigned long)((travelPx + step - 1) / step)) * UserConfig::TEXT_SCROLL_FRAME_MS;
}

void DisplayRenderer::blank() {
  display.fillScreen(COLOR_BLACK);
  resetCache();
}

void DisplayRenderer::drawSparse(const SparseAsset& asset) {
  for (uint16_t i = 0; i < asset.count; i++) {
    AssetPixel p;
    memcpy_P(&p, &asset.pixels[i], sizeof(AssetPixel));
    display.drawPixel(p.x, p.y, scaleColor(p.color));
  }
}

void DisplayRenderer::drawSparseToBaseCache(const SparseAsset& asset) {
  for (uint16_t i = 0; i < asset.count; i++) {
    AssetPixel p;
    memcpy_P(&p, &asset.pixels[i], sizeof(AssetPixel));
    if (p.x < DISPLAY_W && p.y < DISPLAY_H) {
      basePixels[(uint16_t)p.y * DISPLAY_W + p.x] = p.color;
    }
  }
}

void DisplayRenderer::buildBaseCache() {
  for (uint16_t i = 0; i < DISPLAY_W * DISPLAY_H; i++) {
    basePixels[i] = COLOR_BLACK;
  }

  drawSparseToBaseCache(ASSET_MONKEY_BG);
  if (Config::DRAW_STATIC_HEADPHONES_ICON) drawSparseToBaseCache(ASSET_ICON_HEADPHONES);
  if (Config::DRAW_STATIC_SDCARD_ICON) drawSparseToBaseCache(ASSET_ICON_SDCARD);
  if (Config::DRAW_BATTERY_ICON) drawSparseToBaseCache(ASSET_ICON_BATTERY);
}

uint16_t DisplayRenderer::baseColorAt(uint8_t x, uint8_t y) const {
  if (x >= DISPLAY_W || y >= DISPLAY_H) return COLOR_BLACK;
  return basePixels[(uint16_t)y * DISPLAY_W + x];
}

uint16_t DisplayRenderer::scaleColor(uint16_t color) const {
  if (brightnessPercent >= 100 || color == COLOR_BLACK) return color;
  if (brightnessPercent == 0) return COLOR_BLACK;

  uint8_t r5 = (color >> 11) & 0x1F;
  uint8_t g6 = (color >> 5) & 0x3F;
  uint8_t b5 = color & 0x1F;

  r5 = (uint8_t)(((uint16_t)r5 * brightnessPercent) / 100);
  g6 = (uint8_t)(((uint16_t)g6 * brightnessPercent) / 100);
  b5 = (uint8_t)(((uint16_t)b5 * brightnessPercent) / 100);

  return ((uint16_t)r5 << 11) | ((uint16_t)g6 << 5) | b5;
}

void DisplayRenderer::eraseSparse(const SparseAsset& asset) {
  for (uint16_t i = 0; i < asset.count; i++) {
    AssetPixel p;
    memcpy_P(&p, &asset.pixels[i], sizeof(AssetPixel));
    display.drawPixel(p.x, p.y, scaleColor(baseColorAt(p.x, p.y)));
  }
}

void DisplayRenderer::drawBase() {
  display.fillScreen(COLOR_BLACK);
  drawSparse(ASSET_MONKEY_BG);

  if (Config::DRAW_STATIC_HEADPHONES_ICON) drawSparse(ASSET_ICON_HEADPHONES);
  if (Config::DRAW_STATIC_SDCARD_ICON) drawSparse(ASSET_ICON_SDCARD);
  if (Config::DRAW_BATTERY_ICON) drawSparse(ASSET_ICON_BATTERY);
}

void DisplayRenderer::updateBlink(unsigned long nowMs) {
  if (nowMs >= nextBlinkMs) {
    blinkUntilMs = nowMs + Config::BLINK_DURATION_MS;
    nextBlinkMs = nowMs + random(Config::BLINK_EVERY_MIN_MS, Config::BLINK_EVERY_MAX_MS);
  }
}

void DisplayRenderer::eraseFace(FaceMode mode, bool blink, uint8_t faceFrame, const SparseAsset* customFace) {
  if (customFace && mode != FaceMode::Dizzy) {
    eraseSparse(*customFace);
    return;
  }

  if (mode == FaceMode::Dizzy) {
    eraseSparse(ASSET_EYES_DIZZY_1);
    eraseSparse(faceFrame == 0 ? ASSET_MOUTH_DIZZY_1 : ASSET_MOUTH_DIZZY_2);
    eraseSparse(ASSET_NOSE);
    return;
  }

  if (!blink) eraseSparse(ASSET_EYES_OPEN);
  eraseSparse(ASSET_NOSE);
  eraseSparse(ASSET_MOUTH_SMILE);
}

void DisplayRenderer::drawFace(FaceMode mode, bool blink, uint8_t faceFrame, const SparseAsset* customFace) {
  if (customFace && mode != FaceMode::Dizzy) {
    drawSparse(*customFace);
    return;
  }

  if (mode == FaceMode::Dizzy) {
    drawSparse(ASSET_EYES_DIZZY_1);
    drawSparse(faceFrame == 0 ? ASSET_MOUTH_DIZZY_1 : ASSET_MOUTH_DIZZY_2);
    drawSparse(ASSET_NOSE);
    return;
  }

  if (!blink) drawSparse(ASSET_EYES_OPEN);
  drawSparse(ASSET_NOSE);
  drawSparse(ASSET_MOUTH_SMILE);
}

void DisplayRenderer::drawBottomText(const char* text, unsigned long nowMs) {
  if (!text || !text[0]) {
    if (bottomTextMaskValid) {
      for (uint8_t y = 0; y < TEXT_H; y++) {
        for (uint8_t x = 0; x < TEXT_W; x++) {
          uint16_t i = (uint16_t)y * TEXT_W + x;
          if (bottomTextMask[i]) {
            display.drawPixel(TEXT_X + x, TEXT_Y + y, scaleColor(baseColorAt(TEXT_X + x, TEXT_Y + y)));
            bottomTextMask[i] = 0;
          }
        }
      }
      lastBottomText[0] = '\0';
      bottomTextMaskValid = false;
    }
    return;
  }

  bool changed = strncmp(lastBottomText, text, sizeof(lastBottomText)) != 0;
  if (changed) {
    strncpy(lastBottomText, text, sizeof(lastBottomText) - 1);
    lastBottomText[sizeof(lastBottomText) - 1] = '\0';
    bottomTextWidth = TinyFont5::measureText(lastBottomText);
    bottomTextX = -(int16_t)bottomTextWidth;
    nextTextScrollMs = 0;
  }

  if (!changed && nowMs < nextTextScrollMs) return;

  uint8_t nextMask[TEXT_W * TEXT_H] = {0};
  TinyFont5::renderTextMask(lastBottomText, bottomTextX, nextMask, TEXT_W, TEXT_H);

  for (uint8_t y = 0; y < TEXT_H; y++) {
    for (uint8_t x = 0; x < TEXT_W; x++) {
      uint16_t i = (uint16_t)y * TEXT_W + x;
      if (bottomTextMaskValid && bottomTextMask[i] == nextMask[i]) continue;

      uint16_t color = nextMask[i] ? COLOR_WHITE : baseColorAt(TEXT_X + x, TEXT_Y + y);
      display.drawPixel(TEXT_X + x, TEXT_Y + y, scaleColor(color));
      bottomTextMask[i] = nextMask[i];
    }
  }
  bottomTextMaskValid = true;

  bottomTextX += UserConfig::TEXT_SCROLL_STEP_PX;
  if (bottomTextX > TEXT_W) {
    bottomTextX = -(int16_t)bottomTextWidth;
  }
  nextTextScrollMs = nowMs + UserConfig::TEXT_SCROLL_FRAME_MS;
}

void DisplayRenderer::render(FaceMode mode, bool playing, const char* bottomText, const SparseAsset* customFace, unsigned long nowMs) {
  if (!bottomText) bottomText = "";
  if (mode == FaceMode::Dizzy) customFace = nullptr;
  updateBlink(nowMs);

  bool blink = !customFace && mode != FaceMode::Dizzy && nowMs < blinkUntilMs;
  uint8_t faceFrame = 0;
  if (mode == FaceMode::Dizzy) {
    faceFrame = ((nowMs / Config::DIZZY_FRAME_MS) % 2) == 0 ? 0 : 1;
  }

  if (!baseDrawn) {
    drawBase();
    baseDrawn = true;
    lastMode = FaceMode::Sleepy;
    lastFaceFrame = 255;
    lastCustomFace = nullptr;
    lastBottomText[0] = '\0';
  }

  bool faceChanged = lastMode == FaceMode::Sleepy ||
                     mode != lastMode ||
                     faceFrame != lastFaceFrame ||
                     blink != lastBlink ||
                     customFace != lastCustomFace;
  if (faceChanged) {
    if (lastMode != FaceMode::Sleepy) {
      eraseFace(lastMode, lastBlink, lastFaceFrame, lastCustomFace);
    }
    drawFace(mode, blink, faceFrame, customFace);
    lastMode = mode;
    lastFaceFrame = faceFrame;
    lastBlink = blink;
    lastCustomFace = customFace;
  }

  drawBottomText(bottomText, nowMs);

  if (Config::DRAW_PLAYING_INDICATOR && playing != lastPlaying) {
    for (uint8_t y = 60; y < 64; y++) {
      for (uint8_t x = 90; x < 96; x++) {
        display.drawPixel(x, y, scaleColor(baseColorAt(x, y)));
      }
    }
    if (playing) {
      display.drawPixel(91, 61, scaleColor(COLOR_DIM_WHITE));
      display.drawPixel(93, 61, scaleColor(COLOR_DIM_WHITE));
    }
    lastPlaying = playing;
  } else if (!Config::DRAW_PLAYING_INDICATOR) {
    lastPlaying = playing;
  }
}

void DisplayRenderer::showSleepScreen() {
  drawBase();
  drawSparse(ASSET_NOSE);
  TinyFont5::drawText(display, 24, 58, "Bye", scaleColor(COLOR_WHITE), COLOR_BLACK, 48);
  resetCache();
}

void DisplayRenderer::resetCache() {
  baseDrawn = false;
  lastMode = FaceMode::Sleepy;
  lastFaceFrame = 255;
  lastBlink = false;
  lastCustomFace = nullptr;
  lastPlaying = false;
  lastBottomText[0] = '\0';
  memset(bottomTextMask, 0, sizeof(bottomTextMask));
  bottomTextMaskValid = false;
  bottomTextX = 0;
  bottomTextWidth = 0;
  nextTextScrollMs = 0;
}
