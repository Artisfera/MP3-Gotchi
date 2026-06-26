#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

class MotionSensor {
public:
  bool begin(uint8_t sdaPin, uint8_t sclPin);
  bool update();
  bool available() const { return found; }
  uint8_t address() const { return currentAddress; }

private:
  bool beginAtAddress(uint8_t address);

  Adafruit_MPU6050 mpu;
  bool found = false;
  uint8_t currentAddress = 0x68;
  bool hasAccelSample = false;
  float lastAccelMagnitude = 9.80665f;
  unsigned long lastShakeMs = 0;
};
