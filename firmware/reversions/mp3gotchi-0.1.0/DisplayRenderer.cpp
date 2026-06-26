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

  constexpr uint8_t EYES_X = 32;
  constexpr uint8_t EYES_Y = 23;
  constexpr uint8_t EYES_W = 32;
  constexpr uint8_t EYES_H = 12;

  constexpr uint8_t LOWER_FACE_X = 40;
  constexpr uint8_t LOWER_FACE_Y = 37;
  constexpr uint8_t LOWER_FACE_W = 18;
  constexpr uint8_t LOWER_FACE_H = 13;

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

void DisplayRenderer::drawSparseInRect(const SparseAsset& asset, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  uint8_t x2 = x + w;
  uint8_t y2 = y + h;

  for (uint16_t i = 0; i < asset.count; i++) {
    AssetPixel p;
    memcpy_P(&p, &asset.pixels[i], sizeof(AssetPixel));
    if (p.x >= x && p.x < x2 && p.y >= y && p.y < y2) {
      display.drawPixel(p.x, p.y, p.color);
    }
  }
}

void DisplayRenderer::restoreRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  display.fillRect(x, y, w, h, COLOR_BLACK);
  drawSparseInRect(ASSET_MONKEY_BG, x, y, w, h);
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

void DisplayRenderer::drawFace(FaceMode mode, bool blink, uint8_t faceFrame) {
  restoreRect(EYES_X, EYES_Y, EYES_W, EYES_H);
  restoreRect(LOWER_FACE_X, LOWER_FACE_Y, LOWER_FACE_W, LOWER_FACE_H);

  if (mode == FaceMode::Dizzy) {
    drawSparse(ASSET_EYES_DIZZY_1);
    if (faceFrame == 0) drawSparse(ASSET_MOUTH_DIZZY_1);
    else drawSparse(ASSET_MOUTH_DIZZY_2);
    drawSparse(ASSET_NOSE);
    return;
  }

  if (!blink) drawSparse(ASSET_EYES_OPEN);
  drawSparse(ASSET_NOSE);
  drawSparse(ASSET_MOUTH_SMILE);
}

void DisplayRenderer::drawBottomText(const char* text) {
  if (!text || !text[0]) {
    display.fillRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H, COLOR_BLACK);
    return;
  }
  TinyFont5::drawText(display, TEXT_X, TEXT_Y, text, COLOR_WHITE, COLOR_BLACK, TEXT_W);
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

  if (mode != lastMode || faceFrame != lastFaceFrame || blink != lastBlink) {
    drawFace(mode, blink, faceFrame);
    lastMode = mode;
    lastFaceFrame = faceFrame;
    lastBlink = blink;
  }

  if (strncmp(lastBottomText, bottomText, sizeof(lastBottomText)) != 0) {
    drawBottomText(bottomText);
    strncpy(lastBottomText, bottomText, sizeof(lastBottomText) - 1);
    lastBottomText[sizeof(lastBottomText) - 1] = '\0';
  }

  if (Config::DRAW_PLAYING_INDICATOR && playing != lastPlaying) {
    restoreRect(90, 60, 6, 4);
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

void DisplayRenderer::showWakeGuard(uint8_t clicksDone) {
  drawBase();
  drawSparse(ASSET_EYES_OPEN);
  drawSparse(ASSET_NOSE);
  drawSparse(ASSET_MOUTH_SMILE);
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "WAKE %u/5", clicksDone);
  TinyFont5::drawText(display, 0, 58, buffer, COLOR_WHITE, COLOR_BLACK, 96);
  resetCache();
}

void DisplayRenderer::resetCache() {
  baseDrawn = false;
  lastMode = FaceMode::Sleepy;
  lastFaceFrame = 255;
  lastBlink = false;
  lastPlaying = false;
  lastBottomText[0] = '\0';
}
