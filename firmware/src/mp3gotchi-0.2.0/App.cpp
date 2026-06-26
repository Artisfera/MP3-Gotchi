#include "App.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include "UserConfig.h"
#include "UserEmotes.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

namespace {
  constexpr unsigned long TRACK_FINISH_DUPLICATE_IGNORE_MS = 2500;

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
  Serial.println("==================================================");
  Serial.println(" MP3-Gotchi");
  Serial.println(" Firmware v0.2.0");
  Serial.println(" Author: Patryk Ankudowicz (Artisfera)");
  Serial.println(" Code assistant: ChatGPT");
  Serial.println(" License: community use - non commercial");
  Serial.println(" Commands: pinout, brightness, shake, reboot, mpu diag, yx diag");
  Serial.println("==================================================");
  Serial.println();

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
  loadShakeSettings();

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
  setMessage("MP3-Gotchi", 1600);
  renderIfNeeded(true);
}

void App::loop() {
  handleSerialCommands();
  handleAudioEvents();

  InputEvents events = input.update();
  handleInput(events);

  if (motion.update(currentShake)) {
    dizzyUntilMs = millis() + Config::DIZZY_DURATION_MS;
    haptics.play(HapticPattern::Shake);
  }

  updateUserEmote(millis());
  haptics.update();
  renderIfNeeded(false);
}

void App::handleSerialCommands() {
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
  } else if (strcmp(command, "shake") == 0) {
    printShakeSettings();
  } else if (strncmp(command, "shake ", 6) == 0) {
    setShakeSetting(command + 6);
  } else if (strcmp(command, "mpu diag") == 0) {
    runMpuDiagnostics();
  } else if (strcmp(command, "yx diag") == 0) {
    runYxDiagnostics();
  } else if (strcmp(command, "reboot") == 0) {
    Serial.println("Rebooting now.");
    Serial.flush();
    delay(100);
    ESP.restart();
  } else {
    Serial.println("Unknown command. Use: pinout, brightness, brightness 0-100, shake, shake hard|normal|light|verylight|off|on, shake total|change|gyro|confirm|cooldown <value>, reboot, mpu diag, yx diag.");
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
  printShakeSettings();
  Serial.print("Encoder mode: ");
  Serial.println(encoderModeName());
}

void App::printBrightness() const {
  Serial.print("OLED brightness: ");
  Serial.println(currentBrightness);
}

void App::printShakeSettings() const {
  Serial.println("Shake settings:");
  Serial.print("  enabled: ");
  Serial.println(currentShake.enabled ? "yes" : "no");
  Serial.print("  total-g threshold: ");
  Serial.println(currentShake.accelTotalG, 3);
  Serial.print("  change-g threshold: ");
  Serial.println(currentShake.accelChangeG, 3);
  Serial.print("  gyro threshold dps: ");
  Serial.println(currentShake.gyroDps, 1);
  Serial.print("  confirm samples: ");
  Serial.println(currentShake.confirmSamples);
  Serial.print("  cooldown ms: ");
  Serial.println(currentShake.cooldownMs);
  Serial.println("  commands: shake hard, shake normal, shake light, shake verylight, shake off, shake on");
  Serial.println("  custom: shake total <g>, shake change <g>, shake gyro <dps>, shake confirm <1-10>, shake cooldown <ms>");
}

void App::setBrightness(uint8_t brightness) {
  if (brightness > 100) brightness = 100;
  currentBrightness = brightness;
  display.setBrightness(currentBrightness);
  prefs.putUChar("oledBrightness", currentBrightness);
  Serial.print("OLED brightness set to ");
  Serial.println(currentBrightness);
  char text[20];
  snprintf(text, sizeof(text), "Brightness %u", currentBrightness);
  setMessage(text, 1100);
  renderIfNeeded(true);
}

void App::loadShakeSettings() {
  currentShake.enabled = prefs.getBool("shakeEn", UserConfig::SHAKE_ENABLED);
  currentShake.accelTotalG = prefs.getFloat("shakeTotG", UserConfig::SHAKE_ACCEL_TOTAL_G_THRESHOLD);
  currentShake.accelChangeG = prefs.getFloat("shakeChgG", UserConfig::SHAKE_ACCEL_CHANGE_G_THRESHOLD);
  currentShake.gyroDps = prefs.getFloat("shakeGyro", UserConfig::SHAKE_GYRO_DPS_THRESHOLD);
  currentShake.confirmSamples = prefs.getUChar("shakeConf", UserConfig::SHAKE_CONFIRM_SAMPLES);
  currentShake.cooldownMs = prefs.getULong("shakeCool", UserConfig::SHAKE_COOLDOWN_MS);

  if (currentShake.accelTotalG < 0.05f || currentShake.accelTotalG > 3.50f) currentShake.accelTotalG = UserConfig::SHAKE_ACCEL_TOTAL_G_THRESHOLD;
  if (currentShake.accelChangeG < 0.02f || currentShake.accelChangeG > 3.50f) currentShake.accelChangeG = UserConfig::SHAKE_ACCEL_CHANGE_G_THRESHOLD;
  if (currentShake.gyroDps < 10.0f || currentShake.gyroDps > 2000.0f) currentShake.gyroDps = UserConfig::SHAKE_GYRO_DPS_THRESHOLD;
  if (currentShake.confirmSamples < 1 || currentShake.confirmSamples > 10) currentShake.confirmSamples = UserConfig::SHAKE_CONFIRM_SAMPLES;
  if (currentShake.cooldownMs > 10000UL) currentShake.cooldownMs = UserConfig::SHAKE_COOLDOWN_MS;
}

void App::saveShakeSettings() {
  prefs.putBool("shakeEn", currentShake.enabled);
  prefs.putFloat("shakeTotG", currentShake.accelTotalG);
  prefs.putFloat("shakeChgG", currentShake.accelChangeG);
  prefs.putFloat("shakeGyro", currentShake.gyroDps);
  prefs.putUChar("shakeConf", currentShake.confirmSamples);
  prefs.putULong("shakeCool", currentShake.cooldownMs);
}

void App::applyShakeProfile(const char* profile) {
  if (strcmp(profile, "off") == 0) {
    currentShake.enabled = false;
  } else if (strcmp(profile, "on") == 0) {
    currentShake.enabled = true;
  } else if (strcmp(profile, "hard") == 0) {
    currentShake.enabled = true;
    currentShake.accelTotalG = 1.20f;
    currentShake.accelChangeG = 0.75f;
    currentShake.gyroDps = 520.0f;
    currentShake.confirmSamples = 3;
    currentShake.cooldownMs = 1100;
  } else if (strcmp(profile, "normal") == 0) {
    currentShake.enabled = true;
    currentShake.accelTotalG = UserConfig::SHAKE_ACCEL_TOTAL_G_THRESHOLD;
    currentShake.accelChangeG = UserConfig::SHAKE_ACCEL_CHANGE_G_THRESHOLD;
    currentShake.gyroDps = UserConfig::SHAKE_GYRO_DPS_THRESHOLD;
    currentShake.confirmSamples = UserConfig::SHAKE_CONFIRM_SAMPLES;
    currentShake.cooldownMs = UserConfig::SHAKE_COOLDOWN_MS;
  } else if (strcmp(profile, "light") == 0) {
    currentShake.enabled = true;
    currentShake.accelTotalG = 0.55f;
    currentShake.accelChangeG = 0.30f;
    currentShake.gyroDps = 220.0f;
    currentShake.confirmSamples = 2;
    currentShake.cooldownMs = 700;
  } else if (strcmp(profile, "verylight") == 0) {
    currentShake.enabled = true;
    currentShake.accelTotalG = 0.35f;
    currentShake.accelChangeG = 0.18f;
    currentShake.gyroDps = 130.0f;
    currentShake.confirmSamples = 1;
    currentShake.cooldownMs = 600;
  } else {
    Serial.println("Unknown shake profile. Use: hard, normal, light, verylight, off, on.");
    return;
  }

  saveShakeSettings();
  Serial.print("Shake profile set to ");
  Serial.println(profile);
  printShakeSettings();
  setMessage(currentShake.enabled ? "Shake set" : "Shake off", 1100);
  renderIfNeeded(true);
}

void App::setShakeSetting(char* command) {
  if (
    strcmp(command, "off") == 0 ||
    strcmp(command, "on") == 0 ||
    strcmp(command, "hard") == 0 ||
    strcmp(command, "normal") == 0 ||
    strcmp(command, "light") == 0 ||
    strcmp(command, "verylight") == 0
  ) {
    applyShakeProfile(command);
    return;
  }

  if (strncmp(command, "total ", 6) == 0) {
    float value = (float)atof(command + 6);
    if (value < 0.05f || value > 3.50f) {
      Serial.println("Shake total threshold must be 0.05..3.50 g.");
      return;
    }
    currentShake.accelTotalG = value;
  } else if (strncmp(command, "change ", 7) == 0) {
    float value = (float)atof(command + 7);
    if (value < 0.02f || value > 3.50f) {
      Serial.println("Shake change threshold must be 0.02..3.50 g.");
      return;
    }
    currentShake.accelChangeG = value;
  } else if (strncmp(command, "gyro ", 5) == 0) {
    float value = (float)atof(command + 5);
    if (value < 10.0f || value > 2000.0f) {
      Serial.println("Shake gyro threshold must be 10..2000 dps.");
      return;
    }
    currentShake.gyroDps = value;
  } else if (strncmp(command, "confirm ", 8) == 0) {
    int value = atoi(command + 8);
    if (value < 1 || value > 10) {
      Serial.println("Shake confirm samples must be 1..10.");
      return;
    }
    currentShake.confirmSamples = (uint8_t)value;
  } else if (strncmp(command, "cooldown ", 9) == 0) {
    long value = atol(command + 9);
    if (value < 0 || value > 10000) {
      Serial.println("Shake cooldown must be 0..10000 ms.");
      return;
    }
    currentShake.cooldownMs = (unsigned long)value;
  } else {
    Serial.println("Unknown shake command. Use: shake, shake hard|normal|light|verylight|off|on, shake total|change|gyro|confirm|cooldown <value>.");
    return;
  }

  currentShake.enabled = true;
  saveShakeSettings();
  Serial.println("Shake custom setting saved.");
  printShakeSettings();
  setMessage("Shake set", 1100);
  renderIfNeeded(true);
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
  Serial.println("MPU6050 diagnostic start.");
  Serial.print("MPU6050 pinout: SDA GPIO");
  Serial.print(currentMpuSdaPin);
  Serial.print(", SCL GPIO");
  Serial.print(currentMpuSclPin);
  Serial.print(", I2C ");
  Serial.println(currentMpuFrequency);
  printShakeSettings();
  initMotion();
  if (!motionOk) return;

  Serial.println("MPU6050 samples for 10 seconds:");
  for (uint8_t i = 0; i < 10; i++) {
    motion.printSample();
    unsigned long waitStart = millis();
    while (millis() - waitStart < 1000) {
      haptics.update();
      handleAudioEvents();
      delay(5);
    }
  }
}

void App::runYxDiagnostics() {
  Serial.println("YX5300 diagnostic start.");
  Serial.print("YX5300 pinout: ESP RX GPIO");
  Serial.print(currentYxRxPin);
  Serial.print(", ESP TX GPIO");
  Serial.println(currentYxTxPin);
  Serial.print("Volume: ");
  Serial.println(audio.volume());
  Serial.print("Track index offset: ");
  Serial.println(UserConfig::YX_TRACK_INDEX_OFFSET);
  Serial.print("Configured logical track count: ");
  Serial.println(UserConfig::USER_TRACK_COUNT);

  audio.queryTotalTracks();
  audio.queryCurrentTrack();
  unsigned long waitStart = millis();
  while (millis() - waitStart < 700) {
    audio.update();
    delay(5);
  }

  Serial.print("YX5300 total tracks: ");
  Serial.println(audio.totalTracks());
  Serial.print("YX5300 raw module track: ");
  Serial.println(audio.moduleTrack());
  Serial.print("Logical track: ");
  Serial.println(audio.trackKnown() ? audio.track() : 0);
}

void App::initAudioTransport() {
  audio.begin(Serial2, currentYxRxPin, currentYxTxPin);
  Serial.println("YX5300 UART initialized.");
}

void App::handleAudioEvents() {
  Yx5300Player::Event event = audio.update();
  if (event == Yx5300Player::Event::CurrentTrack) {
    Serial.print("YX5300 raw module track: ");
    Serial.print(audio.moduleTrack());
    Serial.print(", logical track: ");
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

  uint16_t finishedModuleTrack = audio.moduleTrack();
  unsigned long now = millis();
  if (
    finishedModuleTrack > 0 &&
    finishedModuleTrack == lastFinishedModuleTrack &&
    now - lastTrackFinishedEventMs < TRACK_FINISH_DUPLICATE_IGNORE_MS
  ) {
    Serial.println("Duplicate track finished packet ignored.");
    return;
  }

  lastFinishedModuleTrack = finishedModuleTrack;
  lastTrackFinishedEventMs = now;

  Serial.print("Track finished, raw module track: ");
  Serial.print(finishedModuleTrack);
  Serial.print(", logical track: ");
  Serial.println(audio.trackKnown() ? audio.track() : 0);
  audio.next();
  Serial.println("Auto next command sent to YX5300.");

  if (audio.trackKnown()) prefs.putUShort("track", audio.track());
  maybeTrackMessage();
  renderIfNeeded(true);
}

void App::resetToFirstTrack() {
  lastFinishedModuleTrack = 0;
  lastTrackFinishedEventMs = 0;
  audio.playTrackNumber(1);
  prefs.putUShort("track", 1);
  encoderMode = EncoderMode::Track;
  setMessage("Track 1", 1400);
  Serial.println("Reset to track 1");
  haptics.play(HapticPattern::Track);
  renderIfNeeded(true);
}

void App::toggleEncoderMode() {
  if (encoderMode == EncoderMode::Track) {
    encoderMode = EncoderMode::Volume;
  } else if (encoderMode == EncoderMode::Volume) {
    encoderMode = EncoderMode::Brightness;
  } else {
    encoderMode = EncoderMode::Track;
  }
  encoderTrackAccum = 0;
  encoderVolumeAccum = 0;
  encoderBrightnessAccum = 0;
  setMessage(encoderModeName(), 1200);
  Serial.print("Encoder mode: ");
  Serial.println(encoderModeName());
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
    char text[16];
    snprintf(text, sizeof(text), "Volume %u", volume);
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
    char text[16];
    snprintf(text, sizeof(text), "Volume %u", volume);
    setMessage(text, 1200);
    Serial.print("Volume: ");
    Serial.println(volume);
    renderIfNeeded(true);
  }
}

void App::handleBrightnessRotation(int8_t detents) {
  encoderBrightnessAccum += detents;

  if (encoderBrightnessAccum >= UserConfig::BRIGHTNESS_CHANGE_DETENTS) {
    encoderBrightnessAccum = 0;
    uint8_t nextBrightness = currentBrightness;
    uint8_t step = UserConfig::BRIGHTNESS_CHANGE_STEP_PERCENT;
    if (step == 0) step = 1;
    if (nextBrightness + step > 100) nextBrightness = 100;
    else nextBrightness += step;
    setBrightness(nextBrightness);
  } else if (encoderBrightnessAccum <= -UserConfig::BRIGHTNESS_CHANGE_DETENTS) {
    encoderBrightnessAccum = 0;
    uint8_t step = UserConfig::BRIGHTNESS_CHANGE_STEP_PERCENT;
    if (step == 0) step = 1;
    uint8_t nextBrightness = currentBrightness > step ? currentBrightness - step : 0;
    setBrightness(nextBrightness);
  }
}

void App::handleInput(const InputEvents& events) {
  if (events.rotationDetents != 0) {
    if (encoderMode == EncoderMode::Volume) {
      handleVolumeRotation(events.rotationDetents);
    } else if (encoderMode == EncoderMode::Brightness) {
      handleBrightnessRotation(events.rotationDetents);
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
    setMessage(audio.isPlaying() ? "Play" : "Pause", 1300);
    renderIfNeeded(true);
  }
}

void App::handleTrackForward() {
  lastFinishedModuleTrack = 0;
  lastTrackFinishedEventMs = 0;
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
    lastFinishedModuleTrack = 0;
    lastTrackFinishedEventMs = 0;
    Serial.println("Previous track command sent to YX5300.");
    maybeTrackMessage();
  } else {
    audio.restartCurrent();
    lastFinishedModuleTrack = 0;
    lastTrackFinishedEventMs = 0;
    Serial.println("Restart current track");
    setMessage("Restart", 1200);
  }

  lastLeftTurnActionMs = now;
  haptics.play(HapticPattern::Track);
  renderIfNeeded(true);
}

const char* App::encoderModeName() const {
  switch (encoderMode) {
    case EncoderMode::Track: return "Track";
    case EncoderMode::Volume: return "Volume";
    case EncoderMode::Brightness: return "Brightness";
  }
  return "Track";
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
  setMessage("Sleep", 600);
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
