#pragma once
#include <Arduino.h>
#include <Wire.h>

class MotionSensor {
public:
  bool begin(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency = 100000);
  bool update();
  bool printSample();
  bool available() const { return found; }
  uint8_t address() const { return currentAddress; }
  uint8_t whoAmI() const { return currentWhoAmI; }

private:
  enum class InitStage : uint8_t {
    Address,
    WhoAmI,
    Wake,
    ReadSample,
    Ok
  };

  bool addressResponds(uint8_t address);
  bool writeReg(uint8_t address, uint8_t reg, uint8_t value);
  bool readReg(uint8_t address, uint8_t reg, uint8_t& value);
  bool readBytes(uint8_t address, uint8_t reg, uint8_t* buffer, size_t len);
  bool readRawSample(uint8_t* raw, size_t len);
  bool beginAtAddress(uint8_t address, InitStage& failedStage, uint8_t& who);
  static const char* stageName(InitStage stage);

  bool found = false;
  uint8_t currentAddress = 0x68;
  uint8_t currentWhoAmI = 0;
  uint32_t currentFrequency = 100000;
  bool hasAccelSample = false;
  float lastAccelTotalG = 1.0f;
  unsigned long lastShakeMs = 0;
};
