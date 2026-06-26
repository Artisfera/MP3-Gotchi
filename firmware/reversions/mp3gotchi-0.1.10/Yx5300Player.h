#pragma once
#include <Arduino.h>

class Yx5300Player {
public:
  enum class Event : uint8_t {
    None,
    TrackFinished,
    CurrentTrack,
    TotalTracks
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
  void queryCurrentTrack();
  void queryTotalTracks();
  void markTrackUnknown();
  void sleep();
  void reset();
  void selectTfCard();

  bool isPlaying() const { return playing; }
  uint8_t volume() const { return currentVolume; }
  uint8_t folder() const { return currentFolder; }
  uint16_t track() const { return currentTrack; }
  uint16_t moduleTrack() const { return currentModuleTrack; }
  uint16_t totalTracks() const { return totalTrackCount; }
  bool trackKnown() const { return currentTrackKnown; }

private:
  void sendCommand(uint8_t command, uint16_t param = 0, bool feedback = false);
  uint16_t logicalToModuleTrack(uint16_t logicalTrack) const;
  uint16_t moduleToLogicalTrack(uint16_t moduleTrack) const;
  uint16_t mappingTrackCount() const;

  HardwareSerial* serial = nullptr;
  bool playing = false;
  uint8_t currentVolume = 18;
  uint8_t currentFolder = 1;
  uint16_t currentTrack = 1;
  uint16_t currentModuleTrack = 1;
  uint16_t totalTrackCount = 0;
  bool currentTrackKnown = true;
  uint8_t rxBuffer[10] = {0};
  uint8_t rxIndex = 0;
};
