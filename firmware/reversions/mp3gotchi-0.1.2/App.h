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
  unsigned long lastLeftTurnActionMs = 0;
  unsigned long dizzyUntilMs = 0;
  unsigned long messageUntilMs = 0;
  unsigned long nextRenderMs = 0;
  char currentMessage[32] = "";
  char serialCommand[40] = "";
  uint8_t serialCommandLen = 0;
  uint8_t currentYxRxPin = 0;
  uint8_t currentYxTxPin = 0;
  uint8_t currentMpuSdaPin = 0;
  uint8_t currentMpuSclPin = 0;
  bool motionOk = false;

  void handleSerialCommands();
  void processSerialCommand(char* command);
  void printPinout() const;
  void toggleMpuPinout();
  void toggleYxPinout();
  void initMotion();
  void initAudioTransport();
  void handleInput(const InputEvents& events);
  void handleTrackForward();
  void handleTrackLeft();
  void setMessage(const char* text, unsigned long durationMs);
  void maybeTrackMessage();
  void goToDeepSleep();
  void renderIfNeeded(bool force = false);
  FaceMode currentFaceMode() const;
};
