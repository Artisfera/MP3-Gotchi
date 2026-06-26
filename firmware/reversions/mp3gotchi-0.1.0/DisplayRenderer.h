#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include "SpriteAssets.h"

enum class FaceMode : uint8_t {
  Idle,
  Music,
  Dizzy,
  Sleepy
};

class DisplayRenderer {
public:
  DisplayRenderer();
  void begin();
  void render(FaceMode mode, bool playing, const char* bottomText, unsigned long nowMs);
  void showSleepScreen();
  void showWakeGuard(uint8_t clicksDone);
  void blank();

private:
  Adafruit_SSD1331 display;
  unsigned long nextBlinkMs = 0;
  unsigned long blinkUntilMs = 0;
  bool baseDrawn = false;
  FaceMode lastMode = FaceMode::Sleepy;
  uint8_t lastFaceFrame = 255;
  bool lastBlink = false;
  bool lastPlaying = false;
  char lastBottomText[32] = {0};

  void drawSparse(const SparseAsset& asset);
  void drawSparseInRect(const SparseAsset& asset, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
  void restoreRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
  void drawBase();
  void drawFace(FaceMode mode, bool blink, uint8_t faceFrame);
  void drawBottomText(const char* text);
  void updateBlink(unsigned long nowMs);
  void resetCache();
};
