#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "DisplayRenderer.h"
#include "Yx5300Player.h"
#include "EncoderInput.h"
#include "MotionSensor.h"
#include "Haptics.h"
#include "MessagePool.h"
#include "PowerManager.h"

enum class EncoderMode : uint8_t {
  Track,
  Volume,
  Brightness
};

class App {
public:
  void begin();
  void loop();

private:
  DisplayRenderer display;
  Yx5300Player audio;
  EncoderInput input;
  MotionSensor motion;
  Haptics haptics;
  MessagePool messages;
  PowerManager power;
  Preferences prefs;

  int encoderTrackAccum = 0;
  int encoderVolumeAccum = 0;
  int encoderBrightnessAccum = 0;
  unsigned long lastLeftTurnActionMs = 0;
  unsigned long dizzyUntilMs = 0;
  unsigned long messageUntilMs = 0;
  unsigned long nextEmoteRollMs = 0;
  unsigned long emoteUntilMs = 0;
  unsigned long nextRenderMs = 0;
  const SparseAsset* currentEmoteAsset = nullptr;
  char currentMessage[64] = "";
  char serialCommand[64] = "";
  uint8_t serialCommandLen = 0;
  uint8_t currentYxRxPin = 0;
  uint8_t currentYxTxPin = 0;
  uint8_t currentMpuSdaPin = 0;
  uint8_t currentMpuSclPin = 0;
  uint32_t currentMpuFrequency = 100000;
  uint8_t currentBrightness = 100;
  EncoderMode encoderMode = EncoderMode::Track;
  bool motionOk = false;

  void handleSerialCommands();
  void processSerialCommand(char* command);
  void printPinout() const;
  void printBrightness() const;
  void runMpuDiagnostics();
  void runYxDiagnostics();
  void setBrightness(uint8_t brightness);
  void initMotion();
  bool tryMotionPinout(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency);
  void initAudioTransport();
  void handleAudioEvents();
  void resetToFirstTrack();
  void toggleEncoderMode();
  void handleInput(const InputEvents& events);
  void handleTrackForward();
  void handleTrackLeft();
  void handleVolumeRotation(int8_t detents);
  void handleBrightnessRotation(int8_t detents);
  const char* encoderModeName() const;
  void setMessage(const char* text, unsigned long durationMs);
  void maybeTrackMessage();
  void updateUserEmote(unsigned long nowMs);
  void goToDeepSleep();
  void renderIfNeeded(bool force = false);
  FaceMode currentFaceMode() const;
};
