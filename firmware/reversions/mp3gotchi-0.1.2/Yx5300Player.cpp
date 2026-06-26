#include "Yx5300Player.h"

void Yx5300Player::begin(HardwareSerial& port, int8_t rxPin, int8_t txPin) {
  if (serial) serial->end();
  serial = &port;
  serial->begin(9600, SERIAL_8N1, rxPin, txPin);
}

void Yx5300Player::init(uint8_t volume, uint8_t folder, uint16_t track, bool autoplay) {
  currentFolder = folder;
  currentTrack = track;
  reset();
  delay(900);
  selectTfCard();
  delay(250);
  setVolume(volume);
  delay(120);

  if (autoplay) {
    playFolderTrack(currentFolder, currentTrack);
  } else {
    playing = false;
  }
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
  if (currentTrack < 999) currentTrack++;
  playing = true;
}

void Yx5300Player::previous() {
  sendCommand(0x02);
  if (currentTrack > 1) currentTrack--;
  playing = true;
}

void Yx5300Player::restartCurrent() {
  playFolderTrack(currentFolder, currentTrack);
}

void Yx5300Player::playTrackNumber(uint16_t track) {
  currentTrack = track;
  sendCommand(0x03, currentTrack);
  playing = true;
}

void Yx5300Player::playFolderTrack(uint8_t folder, uint16_t track) {
  currentFolder = folder;
  currentTrack = track;
  uint16_t param = ((uint16_t)folder << 8) | (track & 0xFF);
  sendCommand(0x0F, param);
  playing = true;
}

void Yx5300Player::setVolume(uint8_t volume) {
  if (volume > 30) volume = 30;
  currentVolume = volume;
  sendCommand(0x06, currentVolume);
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
