#include "App.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include <ctype.h>
#include <string.h>

void App::begin() {
  Serial.begin(Config::SERIAL_BAUD);
  delay(250);

  Serial.println();
  Serial.println("MP3-Gotchi firmware v0.1.2");
  Serial.println("Code created with ChatGPT as programming assistant. Concept and hardware by Patryk Ankudowicz (Artisfera).");
  Serial.println("Temporary debug commands: pinout, odwroc mpu, odwroc yx.");

  currentYxRxPin = Pins::YX_RX_FROM_MODULE;
  currentYxTxPin = Pins::YX_TX_TO_MODULE;
  currentMpuSdaPin = Pins::MPU_SDA;
  currentMpuSclPin = Pins::MPU_SCL;

  power.begin();
  power.markNormalBoot();
  display.begin();
  haptics.begin();
  input.begin();
  messages.begin();

  prefs.begin("mp3gotchi", false);
  uint8_t volume = prefs.getUChar("volume", Config::DEFAULT_VOLUME);
  uint8_t folder = prefs.getUChar("folder", Config::START_FOLDER);
  uint16_t track = prefs.getUShort("track", Config::START_TRACK);

  printPinout();
  initAudioTransport();
  initMotion();

  audio.init(volume, folder, track, Config::AUTOPLAY_ON_BOOT);
  setMessage("MP3-GOTCHI", 1600);
  renderIfNeeded(true);
}

void App::loop() {
  handleSerialCommands();

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
  // TODO(v0.1.3): remove these temporary pinout debug commands after hardware wiring is confirmed.
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
  } else if (strcmp(command, "odwroc mpu") == 0) {
    toggleMpuPinout();
  } else if (strcmp(command, "odwroc yx") == 0) {
    toggleYxPinout();
  } else if (strcmp(command, "odwroc mpu/yx") == 0) {
    toggleMpuPinout();
    toggleYxPinout();
  } else {
    Serial.println("Unknown command. Use: pinout, odwroc mpu, odwroc yx.");
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
  Serial.println(currentMpuSclPin);
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
  audio.setVolume(Config::DEFAULT_VOLUME);
  delay(100);
  audio.playTrackNumber(1);
  printPinout();
}

void App::initMotion() {
  motionOk = motion.begin(currentMpuSdaPin, currentMpuSclPin);
  Serial.print("MPU6050: ");
  Serial.println(motionOk ? "OK" : "NOT FOUND");
}

void App::initAudioTransport() {
  audio.begin(Serial2, currentYxRxPin, currentYxTxPin);
  Serial.println("YX5300 UART initialized.");
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
