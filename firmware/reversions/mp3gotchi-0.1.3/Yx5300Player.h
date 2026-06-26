#pragma once
#include <Arduino.h>

class Yx5300Player {
public:
  enum class Event : uint8_t {
    None,
    TrackFinished
  };

  void begin(HardwareSerial& port, int8_t rxPin, int8_t txPin);
  void init(uint8_t volume, uint8_t folder, uint16_t track, bool autoplay);

  Event update();
  void play();
  void pause();
  void togglePause();
  void next();
  void previous();
  void restartCurrent();
  void playTrackNumber(uint16_t track);
  void playFolderTrack(uint8_t folder, uint16_t track);
  void setVolume(uint8_t volume);
  void sleep();
  void reset();
  void selectTfCard();

  bool isPlaying() const { return playing; }
  uint8_t volume() const { return currentVolume; }
  uint8_t folder() const { return currentFolder; }
  uint16_t track() const { return currentTrack; }

private:
  void sendCommand(uint8_t command, uint16_t param = 0, bool feedback = false);

  HardwareSerial* serial = nullptr;
  bool playing = false;
  uint8_t currentVolume = 18;
  uint8_t currentFolder = 1;
  uint16_t currentTrack = 1;
  uint8_t rxBuffer[10] = {0};
  uint8_t rxIndex = 0;
};
