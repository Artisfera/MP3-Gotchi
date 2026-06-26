#include "App.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include <string.h>

void App::begin() {
  Serial.begin(Config::SERIAL_BAUD);
  delay(250);

  Serial.println();
  Serial.println("MP3-Gotchi firmware v0.1.0");
  Serial.println("Code created with ChatGPT as programming assistant. Concept and hardware by Patryk Ankudowicz (Artisfera).");

  power.begin();
  display.begin();
  haptics.begin();
  input.begin();
  messages.begin();

  prefs.begin("mp3gotchi", false);
  uint8_t volume = prefs.getUChar("volume", Config::DEFAULT_VOLUME);
  uint8_t folder = prefs.getUChar("folder", Config::START_FOLDER);
  uint16_t track = prefs.getUShort("track", Config::START_TRACK);

  audio.begin(Serial2, Pins::YX_RX_FROM_MODULE, Pins::YX_TX_TO_MODULE);

  motionOk = motion.begin();
  Serial.print("MPU6050: ");
  Serial.println(motionOk ? "OK" : "NOT FOUND");

  handleWakeGuard();

  audio.init(volume, folder, track, Config::AUTOPLAY_ON_BOOT);
  setMessage("MP3-GOTCHI", 1600);
  renderIfNeeded(true);
}

void App::handleWakeGuard() {
  if (!Config::REQUIRE_5_CLICKS_AFTER_WAKE || !power.wokeFromLockedSleep()) {
    power.markNormalBoot();
    return;
  }

  Serial.println("Wake guard active. Click encoder 5 times to unlock.");
  if (collectWakeClicks()) {
    Serial.println("Wake confirmed.");
    power.markNormalBoot();
    return;
  }

  Serial.println("Wake not confirmed. Returning to deep sleep.");
  display.blank();
  power.enterDeepSleep();
}

bool App::collectWakeClicks() {
  unsigned long startMs = millis();
  uint8_t clicks = 1;
  bool lastRaw = digitalRead(Pins::ENCODER_SW);
  bool stable = lastRaw;
  bool ignoreFirstRelease = lastRaw == LOW;
  unsigned long lastDebounceMs = millis();
  display.showWakeGuard(clicks);

  while ((millis() - startMs) < Config::WAKE_GUARD_TIMEOUT_MS) {
    haptics.update();

    bool raw = digitalRead(Pins::ENCODER_SW);
    unsigned long now = millis();

    if (raw != lastRaw) {
      lastRaw = raw;
      lastDebounceMs = now;
    }

    if ((now - lastDebounceMs) > Config::DEBOUNCE_MS && raw != stable) {
      stable = raw;
      if (stable == HIGH) {
        if (ignoreFirstRelease) {
          ignoreFirstRelease = false;
        } else {
          clicks++;
        }
        startMs = now;
        if (clicks > Config::SLEEP_CLICK_COUNT) clicks = Config::SLEEP_CLICK_COUNT;
        display.showWakeGuard(clicks);
        haptics.play(HapticPattern::Click);
      }
    }

    if (clicks >= Config::SLEEP_CLICK_COUNT) {
      haptics.play(HapticPattern::Click);
      return true;
    }

    delay(5);
  }

  return false;
}

void App::loop() {
  InputEvents events = input.update();
  handleInput(events);

  if (motion.update()) {
    dizzyUntilMs = millis() + Config::DIZZY_DURATION_MS;
    setMessage("WHOA!", 1000);
    haptics.play(HapticPattern::Shake);
  }

  haptics.update();
  renderIfNeeded(false);
}

void App::handleInput(const InputEvents& events) {
  if (events.rotationDetents != 0) {
    encoderTrackAccum += events.rotationDetents;

    if (encoderTrackAccum >= Config::TRACK_CHANGE_DETENTS) {
      encoderTrackAccum = 0;
      handleTrackForward();
    } else if (encoderTrackAccum <= -Config::TRACK_CHANGE_DETENTS) {
      encoderTrackAccum = 0;
      handleTrackLeft();
    }
  }

  if (events.clickCount > 0) {
    Serial.print("Click count: ");
    Serial.println(events.clickCount);
  }

  if (events.clickCount >= Config::SLEEP_CLICK_COUNT) {
    goToDeepSleep();
    return;
  }

  if (events.clickCount == 1) {
    audio.togglePause();
    haptics.play(HapticPattern::Click);
    setMessage(audio.isPlaying() ? "PLAY" : "PAUSE", 1300);
    renderIfNeeded(true);
  }
}

void App::handleTrackForward() {
  audio.next();
  prefs.putUShort("track", audio.track());
  Serial.print("Next track, estimated track: ");
  Serial.println(audio.track());

  maybeTrackMessage();
  haptics.play(HapticPattern::Track);
  renderIfNeeded(true);
}

void App::handleTrackLeft() {
  unsigned long now = millis();

  if ((now - lastLeftTurnActionMs) <= Config::PREVIOUS_SECOND_TURN_WINDOW_MS) {
    audio.previous();
    prefs.putUShort("track", audio.track());
    Serial.print("Previous track, estimated track: ");
    Serial.println(audio.track());
    maybeTrackMessage();
  } else {
    audio.restartCurrent();
    Serial.println("Restart current track");
    setMessage("RESTART", 1200);
  }

  lastLeftTurnActionMs = now;
  haptics.play(HapticPattern::Track);
  renderIfNeeded(true);
}

void App::maybeTrackMessage() {
  const char* picked = messages.maybePick();
  if (picked && picked[0]) setMessage(picked, Config::MESSAGE_SHOW_MS);
  else setMessage("", 1);
}

void App::setMessage(const char* text, unsigned long durationMs) {
  if (!text) text = "";
  strncpy(currentMessage, text, sizeof(currentMessage) - 1);
  currentMessage[sizeof(currentMessage) - 1] = '\0';
  messageUntilMs = millis() + durationMs;
}

void App::goToDeepSleep() {
  Serial.println("Entering deepest software sleep possible on current hardware.");
  setMessage("SLEEP", 600);
  display.showSleepScreen();
  haptics.play(HapticPattern::Sleep);

  unsigned long waitStart = millis();
  while (millis() - waitStart < 700) {
    haptics.update();
    delay(5);
  }

  audio.sleep();
  haptics.off();
  display.blank();
  power.enterDeepSleep();
}

FaceMode App::currentFaceMode() const {
  unsigned long now = millis();
  if (now < dizzyUntilMs) return FaceMode::Dizzy;
  if (audio.isPlaying()) return FaceMode::Music;
  return FaceMode::Idle;
}

void App::renderIfNeeded(bool force) {
  unsigned long now = millis();
  FaceMode mode = currentFaceMode();

  unsigned long frameMs = Config::IDLE_FRAME_MS;
  if (mode == FaceMode::Music) frameMs = Config::MUSIC_FRAME_MS;
  if (mode == FaceMode::Dizzy) frameMs = Config::DIZZY_FRAME_MS;

  if (!force && now < nextRenderMs) return;
  nextRenderMs = now + frameMs;

  const char* msg = "";
  if (now < messageUntilMs) msg = currentMessage;

  display.render(mode, audio.isPlaying(), msg, now);
}
