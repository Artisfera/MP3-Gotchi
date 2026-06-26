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
  bool motionOk = false;

  void handleWakeGuard();
  bool collectWakeClicks();
  void handleInput(const InputEvents& events);
  void handleTrackForward();
  void handleTrackLeft();
  void setMessage(const char* text, unsigned long durationMs);
  void maybeTrackMessage();
  void goToDeepSleep();
  void renderIfNeeded(bool force = false);
  FaceMode currentFaceMode() const;
};
