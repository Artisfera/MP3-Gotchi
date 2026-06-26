#include "Yx5300Player.h"
#include "ProjectConfig.h"
#include "UserConfig.h"

namespace {
  void printHexByte(uint8_t value) {
    if (value < 0x10) Serial.print('0');
    Serial.print(value, HEX);
  }
}

void Yx5300Player::begin(HardwareSerial& port, int8_t rxPin, int8_t txPin) {
  if (serial) serial->end();
  serial = &port;
  serial->begin(9600, SERIAL_8N1, rxPin, txPin);
}

void Yx5300Player::init(uint8_t volume, uint8_t folder, uint16_t track, bool autoplay) {
  currentFolder = folder;
  currentTrack = track;
  currentModuleTrack = logicalToModuleTrack(currentTrack);
  currentTrackKnown = true;
  reset();
  delay(900);
  selectTfCard();
  delay(250);
  setVolume(volume);
  delay(120);
  queryTotalTracks();
  unsigned long waitStart = millis();
  while (millis() - waitStart < 350) {
    update();
    delay(5);
  }

  if (autoplay) {
    playTrackNumber(currentTrack);
  } else {
    playing = false;
  }
}

Yx5300Player::Event Yx5300Player::update() {
  if (!serial) return Event::None;

  while (serial->available()) {
    uint8_t b = (uint8_t)serial->read();

    if (rxIndex == 0 && b != 0x7E) continue;
    rxBuffer[rxIndex++] = b;

    if (rxIndex < sizeof(rxBuffer)) continue;

    rxIndex = 0;
    if (rxBuffer[0] != 0x7E || rxBuffer[9] != 0xEF) continue;

    uint8_t command = rxBuffer[3];
    uint16_t param = ((uint16_t)rxBuffer[5] << 8) | rxBuffer[6];
    if (Config::YX_DEBUG_PACKETS) {
      Serial.print("YX5300 packet:");
      for (uint8_t i = 0; i < sizeof(rxBuffer); i++) {
        Serial.print(' ');
        printHexByte(rxBuffer[i]);
      }
      Serial.print(" command 0x");
      printHexByte(command);
      Serial.print(" param ");
      Serial.println(param);
    }

    if (command == 0x3C || command == 0x3D || command == 0x3E) {
      if (param > 0) {
        currentModuleTrack = param;
        currentTrack = moduleToLogicalTrack(param);
      }
      currentTrackKnown = param > 0;
      playing = false;
      return Event::TrackFinished;
    }

    if (command == 0x4C) {
      if (param > 0) {
        currentModuleTrack = param;
        currentTrack = moduleToLogicalTrack(param);
        currentTrackKnown = true;
      }
      return Event::CurrentTrack;
    }

    if (command == 0x48) {
      totalTrackCount = param;
      return Event::TotalTracks;
    }
  }

  return Event::None;
}

void Yx5300Player::reset() {
  sendCommand(0x0C);
  playing = false;
}

void Yx5300Player::selectTfCard() {
  sendCommand(0x09, 0x0002);
}

void Yx5300Player::play() {
  sendCommand(0x0D);
  playing = true;
}

void Yx5300Player::pause() {
  sendCommand(0x0E);
  playing = false;
}

void Yx5300Player::togglePause() {
  if (playing) pause();
  else play();
}

void Yx5300Player::next() {
  sendCommand(0x01);
  currentTrackKnown = false;
  playing = true;
  queryCurrentTrack();
}

void Yx5300Player::previous() {
  sendCommand(0x02);
  currentTrackKnown = false;
  playing = true;
  queryCurrentTrack();
}

void Yx5300Player::restartCurrent() {
  playTrackNumber(currentTrack);
}

void Yx5300Player::playTrackNumber(uint16_t track) {
  if (track == 0) track = 1;
  currentTrack = track;
  currentModuleTrack = logicalToModuleTrack(currentTrack);
  currentTrackKnown = true;
  sendCommand(0x03, currentModuleTrack);
  playing = true;
  queryCurrentTrack();
}

void Yx5300Player::playFolderTrack(uint8_t folder, uint16_t track) {
  currentFolder = folder;
  currentTrack = track;
  currentModuleTrack = logicalToModuleTrack(currentTrack);
  currentTrackKnown = true;
  uint16_t param = ((uint16_t)folder << 8) | (currentModuleTrack & 0xFF);
  sendCommand(0x0F, param);
  playing = true;
  queryCurrentTrack();
}

void Yx5300Player::setVolume(uint8_t volume) {
  if (volume > 30) volume = 30;
  currentVolume = volume;
  sendCommand(0x06, currentVolume);
}

void Yx5300Player::queryCurrentTrack() {
  sendCommand(0x4C);
}

void Yx5300Player::queryTotalTracks() {
  sendCommand(0x48);
}

void Yx5300Player::markTrackUnknown() {
  currentTrackKnown = false;
}

void Yx5300Player::sleep() {
  pause();
  sendCommand(0x0A);
}

void Yx5300Player::sendCommand(uint8_t command, uint16_t param, bool feedback) {
  if (!serial) return;

  uint8_t packet[10];
  packet[0] = 0x7E;
  packet[1] = 0xFF;
  packet[2] = 0x06;
  packet[3] = command;
  packet[4] = feedback ? 0x01 : 0x00;
  packet[5] = (param >> 8) & 0xFF;
  packet[6] = param & 0xFF;

  uint16_t sum = packet[1] + packet[2] + packet[3] + packet[4] + packet[5] + packet[6];
  uint16_t checksum = 0 - sum;

  packet[7] = (checksum >> 8) & 0xFF;
  packet[8] = checksum & 0xFF;
  packet[9] = 0xEF;

  serial->write(packet, sizeof(packet));
  serial->flush();
}

uint16_t Yx5300Player::mappingTrackCount() const {
  if (UserConfig::USER_TRACK_COUNT > 0) return UserConfig::USER_TRACK_COUNT;
  return totalTrackCount;
}

uint16_t Yx5300Player::logicalToModuleTrack(uint16_t logicalTrack) const {
  if (logicalTrack == 0) logicalTrack = 1;

  uint16_t count = mappingTrackCount();
  if (count == 0) return logicalTrack + UserConfig::YX_TRACK_INDEX_OFFSET;

  uint16_t zeroBased = (uint16_t)((logicalTrack - 1) % count);
  uint16_t offset = (uint16_t)(UserConfig::YX_TRACK_INDEX_OFFSET % count);
  return (uint16_t)(((zeroBased + offset) % count) + 1);
}

uint16_t Yx5300Player::moduleToLogicalTrack(uint16_t moduleTrack) const {
  if (moduleTrack == 0) return 0;

  uint16_t count = mappingTrackCount();
  if (count == 0) {
    if (moduleTrack > UserConfig::YX_TRACK_INDEX_OFFSET) {
      return moduleTrack - UserConfig::YX_TRACK_INDEX_OFFSET;
    }
    return moduleTrack;
  }

  uint16_t zeroBased = (uint16_t)((moduleTrack - 1) % count);
  uint16_t offset = (uint16_t)(UserConfig::YX_TRACK_INDEX_OFFSET % count);
  return (uint16_t)(((zeroBased + count - offset) % count) + 1);
}
