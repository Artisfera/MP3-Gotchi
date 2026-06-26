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
  void setBrightness(uint8_t percent);
  unsigned long bottomTextPassDurationMs(const char* text) const;
  void render(FaceMode mode, bool playing, const char* bottomText, const SparseAsset* customFace, unsigned long nowMs);
  void showSleepScreen();
  void blank();

private:
  Adafruit_SSD1331 display;
  uint16_t basePixels[96 * 64] = {0};
  unsigned long nextBlinkMs = 0;
  unsigned long blinkUntilMs = 0;
  bool baseDrawn = false;
  FaceMode lastMode = FaceMode::Sleepy;
  uint8_t lastFaceFrame = 255;
  bool lastBlink = false;
  const SparseAsset* lastCustomFace = nullptr;
  bool lastPlaying = false;
  char lastBottomText[32] = {0};
  int16_t bottomTextX = 0;
  uint16_t bottomTextWidth = 0;
  unsigned long nextTextScrollMs = 0;

  void drawSparse(const SparseAsset& asset);
  void drawSparseToBaseCache(const SparseAsset& asset);
  void buildBaseCache();
  uint16_t baseColorAt(uint8_t x, uint8_t y) const;
  void eraseSparse(const SparseAsset& asset);
  void drawBase();
  void eraseFace(FaceMode mode, bool blink, uint8_t faceFrame, const SparseAsset* customFace);
  void drawFace(FaceMode mode, bool blink, uint8_t faceFrame, const SparseAsset* customFace);
  void drawBottomText(const char* text, unsigned long nowMs);
  void updateBlink(unsigned long nowMs);
  void resetCache();
};
