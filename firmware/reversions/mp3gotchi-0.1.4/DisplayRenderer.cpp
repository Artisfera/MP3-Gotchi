#include "DisplayRenderer.h"
#include "PinMap.h"
#include "ProjectConfig.h"
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

void DisplayRenderer::blank() {
  display.fillScreen(COLOR_BLACK);
  resetCache();
}

void DisplayRenderer::drawSparse(const SparseAsset& asset) {
  for (uint16_t i = 0; i < asset.count; i++) {
    AssetPixel p;
    memcpy_P(&p, &asset.pixels[i], sizeof(AssetPixel));
    display.drawPixel(p.x, p.y, p.color);
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

void DisplayRenderer::eraseSparse(const SparseAsset& asset) {
  for (uint16_t i = 0; i < asset.count; i++) {
    AssetPixel p;
    memcpy_P(&p, &asset.pixels[i], sizeof(AssetPixel));
    display.drawPixel(p.x, p.y, baseColorAt(p.x, p.y));
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

void DisplayRenderer::eraseFace(FaceMode mode, bool blink, uint8_t faceFrame) {
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

void DisplayRenderer::drawFace(FaceMode mode, bool blink, uint8_t faceFrame) {
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
    if (lastBottomText[0]) {
      display.fillRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H, COLOR_BLACK);
      lastBottomText[0] = '\0';
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

  display.fillRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H, COLOR_BLACK);
  TinyFont5::drawTextClipped(display, TEXT_X + bottomTextX, TEXT_Y, lastBottomText, COLOR_WHITE, TEXT_X, TEXT_Y, TEXT_W, TEXT_H);

  bottomTextX += Config::TEXT_SCROLL_STEP_PX;
  if (bottomTextX > TEXT_W) {
    bottomTextX = -(int16_t)bottomTextWidth;
  }
  nextTextScrollMs = nowMs + Config::TEXT_SCROLL_FRAME_MS;
}

void DisplayRenderer::render(FaceMode mode, bool playing, const char* bottomText, unsigned long nowMs) {
  if (!bottomText) bottomText = "";
  updateBlink(nowMs);

  bool blink = mode != FaceMode::Dizzy && nowMs < blinkUntilMs;
  uint8_t faceFrame = 0;
  if (mode == FaceMode::Dizzy) {
    faceFrame = ((nowMs / Config::DIZZY_FRAME_MS) % 2) == 0 ? 0 : 1;
  }

  if (!baseDrawn) {
    drawBase();
    baseDrawn = true;
    lastMode = FaceMode::Sleepy;
    lastFaceFrame = 255;
    lastBottomText[0] = '\0';
  }

  if (lastMode == FaceMode::Sleepy) {
    drawFace(mode, blink, faceFrame);
  } else if (lastMode == FaceMode::Dizzy || mode == FaceMode::Dizzy) {
    if (lastMode != mode) {
      eraseFace(lastMode, lastBlink, lastFaceFrame);
      drawFace(mode, blink, faceFrame);
    } else if (faceFrame != lastFaceFrame) {
      eraseSparse(lastFaceFrame == 0 ? ASSET_MOUTH_DIZZY_1 : ASSET_MOUTH_DIZZY_2);
      drawSparse(faceFrame == 0 ? ASSET_MOUTH_DIZZY_1 : ASSET_MOUTH_DIZZY_2);
    }
  } else if (blink != lastBlink) {
    if (blink) eraseSparse(ASSET_EYES_OPEN);
    else drawSparse(ASSET_EYES_OPEN);
  }

  if (mode != lastMode || faceFrame != lastFaceFrame || blink != lastBlink) {
    lastMode = mode;
    lastFaceFrame = faceFrame;
    lastBlink = blink;
  }

  drawBottomText(bottomText, nowMs);

  if (Config::DRAW_PLAYING_INDICATOR && playing != lastPlaying) {
    for (uint8_t y = 60; y < 64; y++) {
      for (uint8_t x = 90; x < 96; x++) {
        display.drawPixel(x, y, baseColorAt(x, y));
      }
    }
    if (playing) {
      display.drawPixel(91, 61, COLOR_DIM_WHITE);
      display.drawPixel(93, 61, COLOR_DIM_WHITE);
    }
    lastPlaying = playing;
  } else if (!Config::DRAW_PLAYING_INDICATOR) {
    lastPlaying = playing;
  }
}

void DisplayRenderer::showSleepScreen() {
  drawBase();
  drawSparse(ASSET_NOSE);
  TinyFont5::drawText(display, 24, 58, "BYE", COLOR_WHITE, COLOR_BLACK, 48);
  resetCache();
}

void DisplayRenderer::resetCache() {
  baseDrawn = false;
  lastMode = FaceMode::Sleepy;
  lastFaceFrame = 255;
  lastBlink = false;
  lastPlaying = false;
  lastBottomText[0] = '\0';
  bottomTextX = 0;
  bottomTextWidth = 0;
  nextTextScrollMs = 0;
}
