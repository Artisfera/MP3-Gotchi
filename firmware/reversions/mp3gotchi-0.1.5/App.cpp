#include "App.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include "UserConfig.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void App::begin() {
  Serial.begin(Config::SERIAL_BAUD);
  delay(250);

  Serial.println();
  Serial.println("MP3-Gotchi firmware v0.1.5");
  Serial.println("Code created with ChatGPT as programming assistant. Concept and hardware by Patryk Ankudowicz (Artisfera).");
  Serial.println("Temporary debug commands: pinout, brightness 0-100, mpu diag, mpu scan, mpu pins <sda> <scl>, odwroc mpu, odwroc yx.");

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

  printPinout();
  initAudioTransport();
  initMotion();

  audio.init(volume, folder, track, Config::AUTOPLAY_ON_BOOT);
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

  haptics.update();
  renderIfNeeded(false);
}

void App::handleSerialCommands() {
  // TODO(v0.1.6): remove these temporary hardware debug commands after wiring is confirmed.
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
  } else if (strncmp(command, "mpu pins ", 9) == 0) {
    char* rest = command + 9;
    uint8_t sda = (uint8_t)strtoul(rest, &rest, 10);
    uint8_t scl = (uint8_t)strtoul(rest, nullptr, 10);
    if (sda == 0 || scl == 0 || sda > 39 || scl > 39) {
      Serial.println("Use: mpu pins <sda> <scl>");
    } else {
      setMpuPins(sda, scl);
    }
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
    Serial.println("Unknown command. Use: pinout, brightness 0-100, mpu diag, mpu scan, mpu pins <sda> <scl>, odwroc mpu, odwroc yx.");
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
}

void App::initAudioTransport() {
  audio.begin(Serial2, currentYxRxPin, currentYxTxPin);
  Serial.println("YX5300 UART initialized.");
}

void App::handleAudioEvents() {
  Yx5300Player::Event event = audio.update();
  if (event != Yx5300Player::Event::TrackFinished) return;

  Serial.print("Track finished: ");
  Serial.println(audio.track());

  uint16_t nextTrack = audio.track() + 1;
  if (UserConfig::USER_TRACK_COUNT > 0 && audio.track() >= UserConfig::USER_TRACK_COUNT) {
    nextTrack = 1;
  }

  if (nextTrack == 0) nextTrack = 1;
  audio.playTrackNumber(nextTrack);
  Serial.print("Auto play next track: ");
  Serial.println(audio.track());

  prefs.putUShort("track", audio.track());
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
  uint16_t nextTrack = audio.track() + 1;
  if (UserConfig::USER_TRACK_COUNT > 0 && audio.track() >= UserConfig::USER_TRACK_COUNT) {
    nextTrack = 1;
  }
  audio.playTrackNumber(nextTrack);
  prefs.putUShort("track", audio.track());
  Serial.print("Next track, estimated track: ");
  Serial.println(audio.track());

  maybeTrackMessage();
  haptics.play(HapticPattern::Track);
  renderIfNeeded(true);
}

void App::handleTrackLeft() {
  unsigned long now = millis();

  if ((now - lastLeftTurnActionMs) <= UserConfig::PREVIOUS_SECOND_TURN_WINDOW_MS) {
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
  if (picked && picked[0]) setMessage(picked, UserConfig::MESSAGE_SHOW_MS);
  else setMessage("", 1);
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

  display.render(mode, audio.isPlaying(), msg, now);
}
