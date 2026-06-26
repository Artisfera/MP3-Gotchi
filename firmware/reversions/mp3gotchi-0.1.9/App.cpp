#include "App.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include "UserConfig.h"
#include "UserEmotes.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

namespace {
  const UserEmoteAsset* pickUserEmote() {
    if (USER_EMOTE_COUNT == 0 || USER_EMOTE_TOTAL_WEIGHT == 0) return nullptr;

    uint16_t r = (uint16_t)random(1, USER_EMOTE_TOTAL_WEIGHT + 1);
    uint16_t acc = 0;
    for (uint8_t i = 0; i < USER_EMOTE_COUNT; i++) {
      acc += USER_EMOTES[i].weight;
      if (r <= acc) return &USER_EMOTES[i];
    }
    return &USER_EMOTES[0];
  }
}

void App::begin() {
  Serial.begin(Config::SERIAL_BAUD);
  delay(250);

  Serial.println();
  Serial.println("MP3-Gotchi firmware v0.1.9");
  Serial.println("Code created with ChatGPT as programming assistant. Concept and hardware by Patryk Ankudowicz (Artisfera).");
  Serial.println("Temporary debug commands: pinout, brightness 0-100, mpu diag, mpu scan, mpu sample, mpu pins <sda> <scl>, yx query, odwroc mpu, odwroc yx.");

  currentYxRxPin = Pins::YX_RX_FROM_MODULE;
  currentYxTxPin = Pins::YX_TX_TO_MODULE;
  currentMpuSdaPin = Pins::MPU_SDA;
  currentMpuSclPin = Pins::MPU_SCL;

  prefs.begin("mp3gotchi", false);
  uint8_t volume = prefs.getUChar("volume", UserConfig::DEFAULT_VOLUME);
  uint8_t folder = prefs.getUChar("folder", Config::START_FOLDER);
  uint16_t track = prefs.getUShort("track", Config::START_TRACK);
  currentBrightness = prefs.getUChar("oledBrightness", UserConfig::DEFAULT_BRIGHTNESS);
  if (currentBrightness > 100) currentBrightness = UserConfig::DEFAULT_BRIGHTNESS;

  power.begin();
  power.markNormalBoot();
  display.begin();
  display.setBrightness(currentBrightness);
  haptics.begin();
  input.begin();
  messages.begin();
  nextEmoteRollMs = millis() + UserConfig::USER_EMOTE_ROLL_INTERVAL_MS;

  printPinout();
  initAudioTransport();
  initMotion();

  audio.init(volume, folder, track, Config::AUTOPLAY_ON_BOOT);
  audio.queryTotalTracks();
  audio.queryCurrentTrack();
  if (Config::YX_ENABLE_LOOP_ALL_FALLBACK) {
    audio.setLoopAll(true);
    Serial.println("YX5300 loop-all fallback enabled.");
  } else {
    audio.setLoopAll(false);
  }
  setMessage("MP3-GOTCHI", 1600);
  renderIfNeeded(true);
}

void App::loop() {
  handleSerialCommands();
  handleAudioEvents();

  InputEvents events = input.update();
  handleInput(events);

  if (motion.update()) {
    dizzyUntilMs = millis() + Config::DIZZY_DURATION_MS;
    setMessage("WHOA!", 1000);
    haptics.play(HapticPattern::Shake);
  }

  updateUserEmote(millis());
  haptics.update();
  renderIfNeeded(false);
}

void App::handleSerialCommands() {
  // TODO(v0.1.9): remove these temporary hardware debug commands after wiring is confirmed.
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (serialCommandLen > 0) {
        serialCommand[serialCommandLen] = '\0';
        processSerialCommand(serialCommand);
        serialCommandLen = 0;
      }
      continue;
    }

    if (serialCommandLen < sizeof(serialCommand) - 1) {
      serialCommand[serialCommandLen++] = c;
    }
  }
}

void App::processSerialCommand(char* command) {
  while (*command == ' ' || *command == '\t') command++;
  size_t len = strlen(command);
  while (len > 0 && (command[len - 1] == ' ' || command[len - 1] == '\t')) {
    command[--len] = '\0';
  }
  for (size_t i = 0; i < len; i++) {
    command[i] = (char)tolower((unsigned char)command[i]);
  }

  if (strcmp(command, "pinout") == 0) {
    printPinout();
  } else if (strcmp(command, "brightness") == 0) {
    printBrightness();
  } else if (strncmp(command, "brightness ", 11) == 0) {
    int value = atoi(command + 11);
    if (value < 0 || value > 100) {
      Serial.println("Brightness must be 0..100.");
    } else {
      setBrightness((uint8_t)value);
    }
  } else if (strcmp(command, "mpu diag") == 0) {
    runMpuDiagnostics();
  } else if (strcmp(command, "mpu scan") == 0) {
    runMpuScan();
  } else if (strcmp(command, "mpu sample") == 0) {
    motion.printSample();
  } else if (strncmp(command, "mpu pins ", 9) == 0) {
    char* rest = command + 9;
    uint8_t sda = (uint8_t)strtoul(rest, &rest, 10);
    uint8_t scl = (uint8_t)strtoul(rest, nullptr, 10);
    if (sda == 0 || scl == 0 || sda > 39 || scl > 39) {
      Serial.println("Use: mpu pins <sda> <scl>");
    } else {
      setMpuPins(sda, scl);
    }
  } else if (strcmp(command, "yx query") == 0) {
    audio.queryTotalTracks();
    audio.queryCurrentTrack();
    Serial.println("YX5300 query commands sent.");
  } else if (strcmp(command, "odwroc mpu") == 0) {
    toggleMpuPinout();
  } else if (strcmp(command, "odwroc yx") == 0) {
    toggleYxPinout();
  } else if (strcmp(command, "odwroc mpu/yx") == 0) {
    toggleMpuPinout();
    toggleYxPinout();
  } else if (strcmp(command, "loop all on") == 0) {
    audio.setLoopAll(true);
    Serial.println("YX5300 loop-all fallback command sent: on");
  } else if (strcmp(command, "loop all off") == 0) {
    audio.setLoopAll(false);
    Serial.println("YX5300 loop-all fallback command sent: off");
  } else {
    Serial.println("Unknown command. Use: pinout, brightness 0-100, mpu diag, mpu scan, mpu sample, mpu pins <sda> <scl>, yx query, odwroc mpu, odwroc yx.");
  }
}

void App::printPinout() const {
  Serial.print("YX5300 pinout: ESP RX GPIO");
  Serial.print(currentYxRxPin);
  Serial.print(", ESP TX GPIO");
  Serial.println(currentYxTxPin);
  Serial.print("MPU6050 pinout: SDA GPIO");
  Serial.print(currentMpuSdaPin);
  Serial.print(", SCL GPIO");
  Serial.print(currentMpuSclPin);
  Serial.print(", I2C ");
  Serial.println(currentMpuFrequency);
  Serial.print("OLED brightness: ");
  Serial.println(currentBrightness);
  Serial.print("Encoder mode: ");
  Serial.println(encoderMode == EncoderMode::Track ? "TRACK" : "VOLUME");
}

void App::printBrightness() const {
  Serial.print("OLED brightness: ");
  Serial.println(currentBrightness);
}

void App::setBrightness(uint8_t brightness) {
  if (brightness > 100) brightness = 100;
  currentBrightness = brightness;
  display.setBrightness(currentBrightness);
  prefs.putUChar("oledBrightness", currentBrightness);
  Serial.print("OLED brightness set to ");
  Serial.println(currentBrightness);
  setMessage("BRIGHT", 900);
  renderIfNeeded(true);
}

void App::toggleMpuPinout() {
  if (currentMpuSdaPin == Pins::MPU_SDA && currentMpuSclPin == Pins::MPU_SCL) {
    currentMpuSdaPin = Pins::MPU_SCL;
    currentMpuSclPin = Pins::MPU_SDA;
  } else {
    currentMpuSdaPin = Pins::MPU_SDA;
    currentMpuSclPin = Pins::MPU_SCL;
  }

  Serial.println("Temporary debug: reversed MPU6050 SDA/SCL pinout.");
  initMotion();
  printPinout();
}

void App::setMpuPins(uint8_t sdaPin, uint8_t sclPin) {
  currentMpuSdaPin = sdaPin;
  currentMpuSclPin = sclPin;
  Serial.println("Temporary debug: manual MPU6050 SDA/SCL pinout.");
  initMotion();
  printPinout();
}

void App::toggleYxPinout() {
  if (currentYxRxPin == Pins::YX_RX_FROM_MODULE && currentYxTxPin == Pins::YX_TX_TO_MODULE) {
    currentYxRxPin = Pins::YX_TX_TO_MODULE;
    currentYxTxPin = Pins::YX_RX_FROM_MODULE;
  } else {
    currentYxRxPin = Pins::YX_RX_FROM_MODULE;
    currentYxTxPin = Pins::YX_TX_TO_MODULE;
  }

  Serial.println("Temporary debug: reversed YX5300 RX/TX pinout.");
  initAudioTransport();
  audio.setVolume(UserConfig::DEFAULT_VOLUME);
  delay(100);
  audio.playTrackNumber(1);
  printPinout();
}

void App::initMotion() {
  const uint32_t speeds[] = {100000, 50000, 400000};
  const uint8_t pinPairs[][2] = {
    {currentMpuSdaPin, currentMpuSclPin},
    {currentMpuSclPin, currentMpuSdaPin}
  };

  motionOk = false;
  for (uint8_t p = 0; p < 2 && !motionOk; p++) {
    for (uint8_t s = 0; s < sizeof(speeds) / sizeof(speeds[0]) && !motionOk; s++) {
      motionOk = tryMotionPinout(pinPairs[p][0], pinPairs[p][1], speeds[s]);
    }
  }

  Serial.print("MPU6050: ");
  Serial.println(motionOk ? "OK" : "NOT FOUND");
  if (motionOk) {
    Serial.print("MPU6050 address: 0x");
    Serial.print(motion.address(), HEX);
    Serial.print(", WHO_AM_I: 0x");
    Serial.println(motion.whoAmI(), HEX);
  }
}

bool App::tryMotionPinout(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency) {
  Serial.print("Trying MPU6050 SDA GPIO");
  Serial.print(sdaPin);
  Serial.print(", SCL GPIO");
  Serial.print(sclPin);
  Serial.print(", I2C ");
  Serial.println(frequency);

  if (!motion.begin(sdaPin, sclPin, frequency)) return false;

  currentMpuSdaPin = sdaPin;
  currentMpuSclPin = sclPin;
  currentMpuFrequency = frequency;
  return true;
}

void App::runMpuDiagnostics() {
  Serial.println("MPU6050 full diagnostic start.");
  initMotion();
}

void App::runMpuScan() {
  const uint32_t speeds[] = {100000, 50000, 400000};
  const uint8_t pinPairs[][2] = {
    {currentMpuSdaPin, currentMpuSclPin},
    {currentMpuSclPin, currentMpuSdaPin}
  };

  for (uint8_t p = 0; p < 2; p++) {
    for (uint8_t s = 0; s < sizeof(speeds) / sizeof(speeds[0]); s++) {
      motion.scanBus(pinPairs[p][0], pinPairs[p][1], speeds[s]);
    }
  }

  if (motionOk) {
    motion.begin(currentMpuSdaPin, currentMpuSclPin, currentMpuFrequency);
  }
}

void App::initAudioTransport() {
  audio.begin(Serial2, currentYxRxPin, currentYxTxPin);
  Serial.println("YX5300 UART initialized.");
}

void App::handleAudioEvents() {
  Yx5300Player::Event event = audio.update();
  if (event == Yx5300Player::Event::CurrentTrack) {
    Serial.print("YX5300 current track: ");
    Serial.println(audio.trackKnown() ? audio.track() : 0);
    if (audio.trackKnown()) prefs.putUShort("track", audio.track());
    return;
  }

  if (event == Yx5300Player::Event::TotalTracks) {
    Serial.print("YX5300 total tracks: ");
    Serial.println(audio.totalTracks());
    if (audio.totalTracks() > 0 && audio.trackKnown() && audio.track() > audio.totalTracks()) {
      Serial.println("Saved track is above module track count. Resetting to track 1.");
      audio.playTrackNumber(1);
      prefs.putUShort("track", 1);
    }
    return;
  }

  if (event != Yx5300Player::Event::TrackFinished) return;

  Serial.print("Track finished, module-reported track: ");
  Serial.println(audio.trackKnown() ? audio.track() : 0);
  audio.next();
  Serial.println("Auto next command sent to YX5300.");

  if (audio.trackKnown()) prefs.putUShort("track", audio.track());
  maybeTrackMessage();
  renderIfNeeded(true);
}

void App::resetToFirstTrack() {
  audio.playTrackNumber(1);
  prefs.putUShort("track", 1);
  encoderMode = EncoderMode::Track;
  setMessage("TRACK 1", 1400);
  Serial.println("Reset to track 1");
  haptics.play(HapticPattern::Track);
  renderIfNeeded(true);
}

void App::toggleEncoderMode() {
  encoderMode = encoderMode == EncoderMode::Track ? EncoderMode::Volume : EncoderMode::Track;
  encoderTrackAccum = 0;
  encoderVolumeAccum = 0;
  setMessage(encoderMode == EncoderMode::Track ? "TRACK" : "VOLUME", 1200);
  Serial.print("Encoder mode: ");
  Serial.println(encoderMode == EncoderMode::Track ? "TRACK" : "VOLUME");
  haptics.play(HapticPattern::Click);
  renderIfNeeded(true);
}

void App::handleVolumeRotation(int8_t detents) {
  encoderVolumeAccum += detents;

  if (encoderVolumeAccum >= UserConfig::VOLUME_CHANGE_DETENTS) {
    encoderVolumeAccum = 0;
    uint8_t volume = audio.volume();
    if (volume < 30) volume++;
    audio.setVolume(volume);
    prefs.putUChar("volume", volume);
    char text[12];
    snprintf(text, sizeof(text), "VOL %u", volume);
    setMessage(text, 1200);
    Serial.print("Volume: ");
    Serial.println(volume);
    renderIfNeeded(true);
  } else if (encoderVolumeAccum <= -UserConfig::VOLUME_CHANGE_DETENTS) {
    encoderVolumeAccum = 0;
    uint8_t volume = audio.volume();
    if (volume > 0) volume--;
    audio.setVolume(volume);
    prefs.putUChar("volume", volume);
    char text[12];
    snprintf(text, sizeof(text), "VOL %u", volume);
    setMessage(text, 1200);
    Serial.print("Volume: ");
    Serial.println(volume);
    renderIfNeeded(true);
  }
}

void App::handleInput(const InputEvents& events) {
  if (events.rotationDetents != 0) {
    if (encoderMode == EncoderMode::Volume) {
      handleVolumeRotation(events.rotationDetents);
    } else {
      encoderTrackAccum += events.rotationDetents;

      if (encoderTrackAccum >= UserConfig::TRACK_CHANGE_DETENTS) {
        encoderTrackAccum = 0;
        handleTrackForward();
      } else if (encoderTrackAccum <= -UserConfig::TRACK_CHANGE_DETENTS) {
        encoderTrackAccum = 0;
        handleTrackLeft();
      }
    }
  }

  if (events.clickCount > 0) {
    Serial.print("Click count: ");
    Serial.println(events.clickCount);
  }

  if (events.clickCount >= Config::RESET_TRACK_CLICK_COUNT) {
    resetToFirstTrack();
    return;
  }

  if (events.clickCount >= Config::SLEEP_CLICK_COUNT) {
    goToDeepSleep();
    return;
  }

  if (events.clickCount == 2) {
    toggleEncoderMode();
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
  Serial.println("Next track command sent to YX5300.");

  maybeTrackMessage();
  haptics.play(HapticPattern::Track);
  renderIfNeeded(true);
}

void App::handleTrackLeft() {
  unsigned long now = millis();

  if ((now - lastLeftTurnActionMs) <= UserConfig::PREVIOUS_SECOND_TURN_WINDOW_MS) {
    audio.previous();
    Serial.println("Previous track command sent to YX5300.");
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
  if (picked && picked[0]) setMessage(picked, UserConfig::MESSAGE_SHOW_MS);
  else setMessage("", 1);
}

void App::updateUserEmote(unsigned long nowMs) {
  if (currentEmoteAsset && nowMs >= emoteUntilMs) {
    currentEmoteAsset = nullptr;
    emoteUntilMs = 0;
    renderIfNeeded(true);
  }

  if (!UserConfig::USER_EMOTES_ENABLED || USER_EMOTE_COUNT == 0) return;

  unsigned long interval = UserConfig::USER_EMOTE_ROLL_INTERVAL_MS;
  if (interval < 1000) interval = 1000;
  if (nextEmoteRollMs == 0) nextEmoteRollMs = nowMs + interval;
  if (nowMs < nextEmoteRollMs) return;
  while (nowMs >= nextEmoteRollMs) nextEmoteRollMs += interval;

  if (currentEmoteAsset) return;
  if (random(0, 100) >= UserConfig::USER_EMOTE_APPEAR_CHANCE_PERCENT) return;

  const UserEmoteAsset* picked = pickUserEmote();
  if (!picked || !picked->asset) return;

  unsigned long duration = picked->durationMs;
  if (duration == 0) duration = UserConfig::USER_EMOTE_DEFAULT_SHOW_MS;
  currentEmoteAsset = picked->asset;
  emoteUntilMs = nowMs + duration;
  Serial.print("User emote: ");
  Serial.println(picked->name ? picked->name : "");
  renderIfNeeded(true);
}

void App::setMessage(const char* text, unsigned long durationMs) {
  if (!text) text = "";
  strncpy(currentMessage, text, sizeof(currentMessage) - 1);
  currentMessage[sizeof(currentMessage) - 1] = '\0';
  unsigned long minDuration = display.bottomTextPassDurationMs(currentMessage);
  if (durationMs < minDuration) durationMs = minDuration;
  messageUntilMs = millis() + durationMs;
}

void App::goToDeepSleep() {
  Serial.println("Entering deepest software sleep possible on current hardware.");
  prefs.putUShort("track", audio.track());
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

  bool messageActive = now < messageUntilMs && currentMessage[0];
  if (messageActive && frameMs > UserConfig::TEXT_SCROLL_FRAME_MS) {
    frameMs = UserConfig::TEXT_SCROLL_FRAME_MS;
  }

  if (!force && now < nextRenderMs) return;
  nextRenderMs = now + frameMs;

  const char* msg = "";
  if (messageActive) msg = currentMessage;

  const SparseAsset* emote = (currentEmoteAsset && now < emoteUntilMs && mode != FaceMode::Dizzy) ? currentEmoteAsset : nullptr;
  display.render(mode, audio.isPlaying(), msg, emote, now);
}
